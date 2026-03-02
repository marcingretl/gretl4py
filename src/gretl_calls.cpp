/*
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "gretl4py.h" /* MUST be at the first position!!! */
#include "gretl4py_utils.h"
#include "gretl4py_packages.h"
#include "gretl_calls.h"
#include "gretl4py_internals.h"
#include <gretl/libset.h>
extern "C" {
    #include <gretl/gretl_string_table.h>
    #include <gretl/gretl_typemap.h>
    #include <gretl/gretl_www.h>
    #include <gretl/gretl_zip.h>
    #include <gretl/interact.h>
}
#include <filesystem>

#if defined(_WIN32)
# include <gretl/gretl_win32.h>
#else
#include <curl/curl.h>
#endif

#if (defined __GNUC__) && !(defined __clang__)
# define COMPILER_IDENT "GCC " __VERSION__
#else
# define COMPILER_IDENT __VERSION__
#endif

// external symbols
extern "C" {
    double bvnorm_cdf (double rho, double a, double b); // from gretl_normal.c
    int update_addons_index (PRN *prn);                 // from addons_utils.c
}

// global variables (some used outside this file)
static int gretl4py_verbose = 0;
static GretlPrint global_printer = GretlPrint();
static gretl_dataset_ptr script_dataset{nullptr};       // dataset for subsequent script calls
static bool nulldata_executed = false;                  // flag that script_dataset was created/modified by 'nulldata' command
std::unordered_set<std::string> user_vars;              // registry of user variables

//////////////////////////////////////////////////
// module related tools
//////////////////////////////////////////////////
void gretl4py_init (void)
{
    // check libgretl version
    if (libgretl_version() < LIBGRETL_MIN_VER) throw std::runtime_error("gretl4py requires libgretl-1.0 version " + std::to_string(LIBGRETL_MIN_VER) + " or higher");

    // initialize libgretl
    libgretl_init();

    // flag the fact that we're coming from python
    gretl_set_python_mode();

    // we set user workdir, gretldir and dotdir
    py::module_ pathlib = py::module_::import("pathlib");
    py::object start = pathlib.attr("Path").attr("cwd")();
    std::string start_dir = py::str(start);

    if (int err = gretl_set_path_by_name("workdir", start_dir.c_str())) handle_gretl_error(err);
    if (const char* env = std::getenv("GRETL_HOME"); env) {
        if (int err = gretl_set_path_by_name("gretldir", env)) handle_gretl_error(err);
        std::filesystem::path dotdir_path = std::filesystem::path(std::string(env)) / "tmp";
        if (int err = gretl_set_path_by_name("dotdir", dotdir_path.string().c_str())) handle_gretl_error(err);
    }

    // read and set GRETL_PLUGIN_PATH if available
    if (const char *env = std::getenv("GRETL_PLUGIN_PATH"); env) set_gretl_plugin_path(env);

    // read and set GRETL4PY_VERBOSE if available
    if (const char *env = std::getenv("GRETL4PY_VERBOSE"); env) gretl4py_verbose = std::atoi(env);

    // force updating addons index
    if (int err = update_addons_index(global_printer.prn_ptr())) {
        global_printer.print();
        handle_gretl_error(err);
    }

    // verbose output
    if (gretl4py_verbose) std::printf("libgretl initialized\n");
}

void gretl4py_cleanup (void)
{
    py::gil_scoped_acquire gil;
    int err = 0;

    // make sure all C++ objects are destroyed while libgretl still a lives
    for (const auto &name : user_vars) {
        err = user_var_delete_by_name(name.c_str(), NULL);
        if (err) {
            std::cerr << "Failed to delete '" + name + "': ";
            std::cerr << errmsg_get_with_default(err) << std::endl;
        }
#if GRETL_PYBIND11_DEBUG
         else {
            std::cout << "Variable '" + name + "' deleted" << std::endl;
        }
#endif // GRETL_PYBIND11_DEBUG
    }

    GretlPackage::clear_registry();
    libgretl_cleanup();
    if (gretl4py_verbose) std::printf("libgretl cleaned up\n");
}

GretlBundlePtr about (bool do_print)
{
    static const std::unordered_set<std::string> lgretl_strings = { "dotdir", "gnuplot", "gretldir", "seats", "tramo", "tramodir", "workdir", "x12a", "x12adir" };
    GretlBundlePtr gb_strings = std::make_shared<GretlBundle>();
    GretlBundlePtr res = std::make_shared<GretlBundle>();
    std::string mod_ver, lg_ver, py11_ver;
    gretl_bundle *b = NULL;
    int err = 0;

    b = get_sysinfo_bundle(&err);
    if (err) handle_gretl_error(err);
    GretlBundlePtr gb_sysinfo = std::make_shared<GretlBundle>(b);

    gretl_push_c_numeric_locale();

    mod_ver = std::string(GRETL4PY_VERSION).substr(0,4);
    py11_ver = std::to_string(PYBIND11_VERSION_MAJOR);
    py11_ver += "." + std::to_string(PYBIND11_VERSION_MINOR);
    py11_ver += "." + std::to_string(PYBIND11_VERSION_PATCH);
    lg_ver = std::to_string(libgretl_version());

    // built-in strings
    for (const auto &it : lgretl_strings) {
        auto val = std::string(get_built_in_string_by_name(it.c_str()));

        gb_strings->set_element<std::string>(it, val);
    }

    res->set_element<std::string>(std::string("mod_ver"), mod_ver);
    res->set_element<std::string>(std::string("py11_ver"), py11_ver);
    res->set_element<std::string>(std::string("lg_ver"), lg_ver);
    if (gb_sysinfo->get_err()) {
        std::cerr << errmsg_get_with_default(gb_sysinfo->get_err()) << std::endl;
    } else {
        res->set_element<GretlBundlePtr>(std::string("sysinfo"), gb_sysinfo );
    }
    res->set_element<GretlBundlePtr>(std::string("built_in_strings"), gb_strings );

    if (do_print || gretl4py_verbose) {
        std::cout << gretl4py_logo() << std::endl;
        std::cout << "gretl4py v" << mod_ver;
        std::cout << " [" << COMPILER_IDENT << "]" << std::endl;
        std::cout << "\t+ pybind11     v" << py11_ver << std::endl;
        std::cout << "\t+ libgretl-1.0 v" << lg_ver << std::endl;
        if (!gb_sysinfo->get_err()) {
            std::cout << std::endl << "libgretl $sysinfo:" << std::endl;
            gb_sysinfo->print<void>();
        }
        std::cout << "libgretl built-in strings:" << std::endl;
        gb_strings->print<void>();
        py::print("gretl_plugin_path = \"" + std::string(gretl_plugin_path()) + "\"\n");
    }

    gretl_pop_c_numeric_locale();
    if (res->get_err()) std::cerr << errmsg_get_with_default(res->get_err()) << std::endl;

    return res;
}

//////////////////////////////////////////////////
// static functions
//////////////////////////////////////////////////
static std::string extract_varname (std::string formula)
{
    //
    // extract user var name from formula
    // note: if used only in one place (gretl_generate) must be rewritten as lambda!!!
    //
    static const std::array<std::string_view, 7> keywords{ "arrays", "bundle", "list", "matrix", "scalar", "series", "string" };

    gretl4py_trim(formula);

    // step 1: remove leading keyword (if present)
    std::istringstream iss(formula);
    std::string first, second;
    iss >> first >> second;

    if (std::find(keywords.begin(), keywords.end(), first) != keywords.end() && !second.empty()) {
        formula = formula.substr(formula.find(second));
        gretl4py_trim(formula);
    }

    // step 2: if '=' is present, extract varname before it
    const size_t eql_pos = formula.find('=');
    std::string varname;

    if (eql_pos != std::string::npos) {
        varname = formula.substr(0, eql_pos);
    } else {
        varname = formula;
    }

    return gretl4py_trim(varname);
}

//////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////
//
// GretlPrint class methods
//
GretlPrint::GretlPrint (PrnType prn_type) : prn_type_(prn_type)
{
    prn_ = gretl_print_new(this->prn_type_, &err_);
    if (err_) handle_gretl_error(err_);
}

GretlPrint::~GretlPrint (void)
{
    if (prn_ != NULL) gretl_print_destroy(prn_);
}

PRN *GretlPrint::prn_ptr (void)
{
    // at first we always reset a buffer
    if (prn_type_ == GRETL_PRINT_BUFFER) {
        err_ = gretl_print_reset_buffer(prn_);
        if (err_) handle_gretl_error(err_);
    }

    return prn_;
}

void GretlPrint::print (void)
{
    const char *buf = gretl_print_get_buffer(prn_);

    if (buf != NULL && *buf != '\0') py::print(buf); //as of 2025-08-22 we don't print empty line
}

const char *GretlPrint::get_buffer (void)
{
    return gretl_print_get_buffer(prn_);
}

//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////
template <typename T>
gretlopt unpack_kwargs_command (GretlCmdIndex ci, T &dataset, const py::kwargs &kwargs)
{
    //
    // unpack kwargs for libgretl's command interfaces
    // note: for tests, there is dedicated unpack_kwargs_test()
    // important: if dataset is not required, please call with 'std::nullptr_t dataset = nullptr;'!!!
    //
    static_assert(std::is_same_v<T, GretlDatasetPtr> || std::is_same_v<T, std::nullptr_t>, "dataset must be GretlDatasetPtr or std::nullptr_t");
    gretlopt opt = OPT_NONE;

    if (py::len(kwargs) > 0) {
        for (auto &item : kwargs) {
            if (!is_pystr(item.first)) throw std::invalid_argument("option key must be a string");
            const std::string &key = item.first.cast<std::string>();

            // special case: we use GretlDataset class object passed by kwargs
            if constexpr (std::is_same_v<T, GretlDatasetPtr>) {
                if (key == "data") {
                    dataset = item.second.cast<GretlDatasetPtr>();
                    continue;
                }
            }
            if (key == "plot") {
                throw std::invalid_argument("'plot' option not available");
            } else {
                gretl_valid_long_opt(opt, ci, key, item.second);
            }
        }
    }

    // we force --plot=none for certain commands
    if (ci == XCORRGM || ci == CORR) gretl_valid_long_opt(opt, ci, "plot", py::handle(), "none");

    // if we didn't get dataset, let use global
    if constexpr (std::is_same_v<T, GretlDatasetPtr>) {
        if (!dataset) dataset = GretlDataset::default_dataset();
    }

    return opt;
}
template gretlopt unpack_kwargs_command(GretlCmdIndex, std::nullptr_t&, const py::kwargs&);

void do_set (const std::string &key, const std::string &value, const py::kwargs &kwargs)
{
    //
    // interface to libgretl's set
    //
    GretlDatasetPtr dataset = nullptr;
    int err = 0;

    // we unpack kwargs at first
    gretlopt opt = unpack_kwargs_command(SET, dataset, kwargs);

    // we do the job
    err = execute_set(key.empty() ? NULL : key.c_str(), value.empty() ? NULL : value.c_str(),
                        dataset ? dataset->dset : NULL, opt, global_printer.prn_ptr());
    global_printer.print();
    if (err) handle_gretl_error(err);
}

void do_gretl_summary (const std::string &varlist, const py::kwargs &kwargs)
{
    //
    // this function mimics libgretl's 'summary' command
    //
    GretlDatasetPtr dataset = nullptr;
    gretl_list_ptr varlist_ptr{nullptr};
    int err = 0;

    // we unpack kwargs at first
    // Note: dataset is set here
    gretlopt opt = unpack_kwargs_command(SUMMARY, dataset, kwargs);
    if (!dataset) handle_gretl_error(E_NODATA);

    // be build gretl_list
    if (!varlist.empty()) varlist_ptr = list_from_formula(varlist, dataset->dset, err);

    // this code was taken from 'interact.c'
    err = incompatible_options(opt, static_cast<gretlopt>(OPT_B | OPT_W));
    if (err) handle_gretl_error(err);
    if (opt & OPT_B) {
        err = summary_statistics_by(varlist_ptr.get(), dataset->dset, opt, global_printer.prn_ptr());
    } else if (opt & OPT_X) {
        err = matrix_command_driver(SUMMARY, varlist_ptr.get(), kwargs["matrix"].cast<std::string>().c_str(),
                                      dataset->dset, opt, global_printer.prn_ptr());
    } else {
        err = list_summary_driver(varlist_ptr.get(), dataset->dset, opt, global_printer.prn_ptr());
    }
    global_printer.print();
    if (err) handle_gretl_error(err);
}

// T = { GretlDataset ; void }
template <typename T>
void do_gretl_print (const std::string &varlist, const py::kwargs &kwargs)
{
    //
    // this function mimics libgretl's 'print' command
    // note: this is a general printer, so some classes may have own implementations
    //
    GretlDatasetPtr dataset = nullptr;
    gretl_list_ptr varlist_ptr{nullptr};
    gretlopt opt = OPT_NONE;
    int err = 0;

    // lambda for generating list of variables to print
    auto make_varlist = [&varlist, &dataset, &err](void) -> gretl_list_ptr
    {
        gretl_list_ptr varlist_ptr{nullptr};

        if (varlist.empty()) {
            // "print " i.e. without options given
            varlist_ptr.reset(full_var_list(dataset->dset, NULL));
        } else {
            varlist_ptr = list_from_formula(varlist, dataset->dset, err);
        }

        return varlist_ptr;
    };

    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs_command(PRINT, dataset, kwargs);
    } else {
        // if we don't have kwargs, we must assign dataset manually
        dataset = GretlDataset::default_dataset();
    }

    // define object to print
    if constexpr (std::is_same_v<T, GretlDataset>) {
        // for GretlDataset, we _do not_ print user-define variables
        varlist_ptr = make_varlist();
    } else {
        // we look for variables in dataset _only_ if there is no uservar
        if (get_user_var_by_name(varlist.c_str()) == NULL && dataset) varlist_ptr = make_varlist();
    }
    if (err) handle_gretl_error(err);

    // based on 'interact.c' (but we don't allow for string literals)
    if (opt & OPT_L) {
        err = printdata(NULL, varlist.c_str(), dataset ? dataset->dset : NULL,
                        OPT_NONE, global_printer.prn_ptr());
    } else {
        err = printdata(varlist_ptr.get(), varlist_ptr ? NULL : varlist.c_str(),
                        dataset ? dataset->dset : NULL, opt,
                        global_printer.prn_ptr());
    }
    global_printer.print();
    if (err) handle_gretl_error(err);
}
template void do_gretl_print<void>(const std::string&, const py::kwargs&);
template void do_gretl_print<GretlDataset>(const std::string&, const py::kwargs&);

double do_gretl_get_pvalue (const std::string &dist, double value,
                            const py::array_t<double> &params)
{
    //
    // An interface to libgretl's gretl_get_pvalue
    //
    NumPyArrayView arr_view;
    auto dist_code = dist_code_from_string(dist.substr(0, 1).c_str());

    NumPyArray_extract(params, arr_view);
    if (dist_code == static_cast<int>(D_NONE)) {
        throw std::invalid_argument("unknown distribution code: '" + dist + "'");
    } else if (dist_code != static_cast<int>(D_NORMAL) && !params.size()) {
        throw std::invalid_argument("given distribution requires params");
    } else if ((std::min)(arr_view.cols, arr_view.rows) > 1) {
        throw std::invalid_argument("params must be a vector");
    }

    return gretl_get_pvalue(dist_code, params.data(), value);
}

void do_gretl_varlist (const py::kwargs &kwargs)
{
    //
    // function mimics libgretl's 'varlist' command
    //
    GretlDatasetPtr dataset = nullptr;

    auto opt = unpack_kwargs_command(VARLIST, dataset, kwargs);

    // we o the job (based on interact.c)
    if (opt & OPT_T) {
        list_user_vars_of_type(dataset ? dataset->dset : NULL, global_printer.prn_ptr());
    } else if (opt & OPT_S) {
        print_scalars(global_printer.prn_ptr());
    } else if (opt & OPT_A) {
        list_ok_dollar_vars(dataset ? dataset->dset : NULL, global_printer.prn_ptr());
    } else {
        if (!dataset) handle_gretl_error(E_NODATA);
        list_series(dataset->dset, opt, global_printer.prn_ptr());
    }
    global_printer.print();
}

py::object do_gretl_include (const std::string &filename)
{
    //
    // this function mimics libgretl's 'include' command
    // for now it acts as factory function for GretlPackage class
    //
    std::filesystem::path file_path(filename);
    std::string basename = file_path.stem().string();
    std::string suffix = (file_path.extension() == ".gfn" || file_path.extension() == ".zip")
                        ? file_path.extension().string()
                        : ""; // we check if we deal with suffix-ed string or just basename

    // we load the package
    auto pkg = GretlPackage::load_package(basename, suffix);

    // get list of exported function names
    py::object obj = py::cast(pkg);
    py::object func_list = obj.attr("get_functions")();

    // import the 'gretl' module dynamically
    py::module_ gretl_mod = py::module_::import("gretl");

    // for each function, attach a Python callable:
    // pkg.fname = lambda **kwargs: ::_function_exec(fname, **kwargs)
    for (const py::handle &py_fname : func_list) {
        py::str fname = py::str(py_fname);

        // create a wrapper bound to this function name and obj
        py::object wrapper = py::cpp_function (
            [obj, fname](const py::args &args, const py::kwargs &kwargs) -> py::object
            {
                return obj.attr("_function_exec")(fname, *args, **kwargs);
            });

        // set it as an attribute on the object
        gretl_mod.attr(fname) = wrapper;
    }

    return obj;
}

void do_gretl_pkg (const std::string &action, const std::string &pkgname,
                   const py::kwargs &kwargs)
{
    //
    // function which mimics libgretl's 'pkg' command
    // Note: as for now we support only basic operations
    //
    auto gretl_package_check_dependencies = [](const std::filesystem::path &localfile) -> int
    {
        //
        // based on code in 'interact.c'
        // note: localfile is a full path to already installed package (gfn or zip)
        //
        strings_array_ptr depends(nullptr, StringsArrayDeleter{});
        std::filesystem::path gfnname;                              // full path to .gfn
        int ndeps, err = 0;

        // map zip -> gfn
        if (localfile.extension() == ".zip") {
            gfnname = localfile.parent_path() / localfile.stem() / (localfile.stem().string() + ".gfn");
        } else {
            gfnname = localfile;
        }
        char **raw_ptr = package_peek_dependencies(gfnname.string().c_str(), &ndeps);
        if (raw_ptr) {
            depends.get_deleter().nstrs = ndeps;
            depends.reset(raw_ptr);
        }

        // we do the job (install dependencies)
        if (depends) {
            for (int i=0; i<ndeps && !err; ++i) {
                gretl_gchar_ptr dependency_path(gretl_function_package_get_path(depends.get()[i], PKG_ALL));
                if (!dependency_path) do_gretl_pkg("install", std::string(depends.get()[i]), py::dict());
            }
        }

        return err;
    };

    std::nullptr_t fake_dataset = nullptr;
    std::filesystem::path pkgname_as_path(pkgname);
    int err = 0;

    // we check options
    auto opt = unpack_kwargs_command(PKG, fake_dataset, kwargs);

    err = incompatible_options(opt, static_cast<gretlopt>(OPT_L | OPT_S));
    if (err) handle_gretl_error(err);

    // we allow only .zip or .gfn (lowercase!!!)
    std::string suffix = pkgname_as_path.extension().string();

    if (!suffix.empty() && suffix != ".zip" && suffix != ".gfn") throw std::invalid_argument("unsupported file type: " + suffix);

    if (action == "install") {
        gretl_gchar_ptr pkgname_remote_c{nullptr}; // to avoid memory-leak when got 'char *'
        std::filesystem::path localfile;
        std::string gretldir = get_built_in_string_by_name("gretldir");
        std::filesystem::path functions_dir = std::filesystem::path(gretldir) / "functions";
        bool local = (opt & OPT_L) ? true : false;
        bool staging = (opt & OPT_S) ? true : false;

        // installation
        if (local) {
            // local installation
            localfile = functions_dir / pkgname_as_path.filename();
            py::print("installing package:", pkgname, "->", localfile.string());
            //pprintf(prn, _("Installed %s\n"), pkgname);
            err = gretl_copy_file(pkgname.c_str(), localfile.string().c_str());
        } else {
            // we retrieve package from SF
            pkgname_remote_c.reset(retrieve_remote_pkg_filename(pkgname.c_str(), &err));
            if (err) handle_gretl_error(err);
            std::string pkgname_remote(pkgname_remote_c.get());

            localfile = functions_dir / pkgname_remote;
            py::print("installing:", pkgname_remote, "->", localfile.string());
            err = retrieve_remote_function_package(pkgname_remote.c_str(),
                                                   localfile.string().c_str(), staging);
        }
        if (err) handle_gretl_error(err);

        // special handling of .zip-s
        if (localfile.extension() == ".zip") {
            err = gretl_unzip_into(localfile.string().c_str(), functions_dir.string().c_str());
            if (err) handle_gretl_error(err);
            err = gretl_remove(localfile.string().c_str());
            if (err) handle_gretl_error(err);
        }

        // we install dependencies
        err = gretl_package_check_dependencies(localfile);
        if (err) handle_gretl_error(err);

        py::print("package installed successfully:", local ? pkgname_as_path.filename().string() : pkgname);
    } else if (action == "query") {
        auto pkg = GretlPackage::load_package(pkgname, "", false);
        py::print(pkg->repr_impl());
    } else {
        throw std::invalid_argument("unsupported action: " + action);
    }
}

void do_gretl_run (const std::string &filename)
{
    //
    // function mimics libgretl's 'run' command
    //
    std::filesystem::path path{filename};

    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) handle_gretl_error(E_FOPEN, filename);

    gretl_set_batch_mode(1);
    py::print("*** batch? ", gretl_in_batch_mode());
    gretl_execute_script<std::string>("run " + path.string());
}

template <GretlFunIndex FI>
double gretl_get_pvalue_critval_pdf_cdf (const std::string &dist, double value,
                                         const py::object &param)
{
    //
    // general interface to selected libgretl functions from pvalues.c
    //
    // we check if we got one char
    if (dist.size() != 1) throw std::invalid_argument("'dist' takes just one character");

    // we look for distribution code (for "t", "z", etc.)
    const int dist_code = dist_code_from_string(dist.c_str());

    if (dist_code == static_cast<int>(D_NONE)) throw std::invalid_argument("unknown distribution code: '" + dist + "'");

    // we normalize params into either a view over a NumPy double array
    // or a std::vector<double> we fill from scalars/sequences.
    const double *params_ptr = nullptr;
    std::vector<double> params_vec;

    // normalize Python input -> double*
    if (param.is_none()) {
        const double val = 0.0;

        params_ptr = &val;  // we fake libgretl a little
    } else if (is_pynumarray(param)) {
        NumPyArrayView arr_view;
        DNumPyArrayFC arr = py::cast<py::array>(param);

        NumPyArray_extract(arr, arr_view);
        if ((std::min)(arr_view.cols, arr_view.rows) > 1) throw std::invalid_argument("param must be a vector");
        if (arr.size() > 0) params_ptr = arr.data();
    } else if (is_pynumeric(param)) {
        params_vec.push_back(py::cast<double>(param));
        params_ptr = params_vec.data();
    } else if (is_pysequence(param)) {
        py::sequence seq = py::cast<py::sequence>(param);

        params_vec.reserve(seq.size());
        for (py::handle item : seq) {
            params_vec.push_back(py::cast<double>(item));
        }
        if (!params_vec.empty()) params_ptr = params_vec.data();
    } else {
        throw std::invalid_argument("params must be None, int, float, list/tuple, or NumPy array");
    }

    // if the distribution requires parameters, enforce presence
    if (dist_code != static_cast<int>(D_NORMAL) && !params_ptr) throw std::invalid_argument("given distribution requires params");

    // actual work
    if constexpr (FI == GretlFunIndex::F_PVAL) {
        return gretl_get_pvalue(dist_code, params_ptr, value);
    } else if constexpr (FI == GretlFunIndex::F_CRIT) {
        return gretl_get_critval(dist_code, params_ptr, value);
    } else if constexpr (FI == GretlFunIndex::F_PDF) {
        return gretl_get_pdf(dist_code, params_ptr, value);
    } else if constexpr (FI == GretlFunIndex::F_CDF) {
        if (dist_code == D_BINORM) {
            if (params_vec.size() != 2) handle_gretl_error(E_ARGS);
            return bvnorm_cdf(value, params_vec.at(0), params_vec.at(1));
        } else {
            return gretl_get_cdf(dist_code, params_ptr, value);
        }
    } else if constexpr (FI == GretlFunIndex::F_INVCDF) {
        return gretl_get_cdf_inverse(dist_code, params_ptr, value);
    }
}

// T = { py::array_t<double> ; std::string }
template <GretlFunIndex FI, typename T>
py::array_t<double> matrix_functions_wrapper (const std::string &dist, const T &src,
                                              const py::object &param)
{
    //
    // this is a wrapper for matrix operations for certain libgretl functions
    // note: function to call must be added to lambda
    //
    //lambda for selection underlying libgretl function
    auto F = [&](double x)
    {
        if constexpr (FI == GretlFunIndex::F_PDF) {
            return gretl_get_pvalue_critval_pdf_cdf<GretlFunIndex::F_PDF>(dist, x, param);
        } else if constexpr (FI == GretlFunIndex::F_CDF) {
            return gretl_get_pvalue_critval_pdf_cdf<GretlFunIndex::F_CDF>(dist, x, param);
        } else if constexpr (FI == GretlFunIndex::F_PVAL) {
            return gretl_get_pvalue_critval_pdf_cdf<GretlFunIndex::F_PVAL>(dist, x, param);
        } else if constexpr (FI == GretlFunIndex::F_CRIT) {
            return gretl_get_pvalue_critval_pdf_cdf<GretlFunIndex::F_CRIT>(dist, x, param);
        } else if constexpr (FI == GretlFunIndex::F_INVCDF) {
            return gretl_get_pvalue_critval_pdf_cdf<GretlFunIndex::F_INVCDF>(dist, x, param);
        } else {
            static_assert(always_false<T>, "Unhandled GretlFunIndex");
        }
    };

    py::array_t<double> src_mat, res;

    // handle type T
    if constexpr (std::is_same_v<T, py::array_t<double>>) {
        if (src.ndim() > 2) throw std::invalid_argument("given matrix has too many dimensions");
        src_mat = src;
    } else if constexpr (std::is_same_v<T, std::string>) {
        auto dataset = GretlDataset::default_dataset();

        if (!dataset) handle_gretl_error(E_DATA);
        src_mat = dataset->get_series(src);
    } else {
        static_assert(always_false<T>, "matrix_functions_wrapper(): unsupported type T");
    }

    // determine shape
    ssize_t n = src_mat.shape(0);
    ssize_t k = (src_mat.ndim() == 2) ? src_mat.shape(1) : 0;

    // allocate result array and fill it with numbers
    if (k == 0) {
        res = py::array_t<double>(n);
        auto r = res.mutable_unchecked<1>();
        auto src = src_mat.unchecked<1>();

        for (ssize_t i = 0; i < n; ++i) {
            r(i) = F(src(i));
        }
    } else {
        res = py::array_t<double>({n, k});
        auto r = res.mutable_unchecked<2>();
        auto src = src_mat.unchecked<2>();

        for (ssize_t i = 0; i < n; ++i) {
            for (ssize_t j = 0; j < k; ++j) {
                r(i, j) = F(src(i, j));
            }
        }
    }

    return res;
}
template py::array_t<double> matrix_functions_wrapper<GretlFunIndex::F_PDF, py::array_t<double>>(const std::string&, const py::array_t<double>&, const py::object&);
template py::array_t<double> matrix_functions_wrapper<GretlFunIndex::F_CDF, py::array_t<double>>(const std::string&, const py::array_t<double>&, const py::object&);
template py::array_t<double> matrix_functions_wrapper<GretlFunIndex::F_PVAL, py::array_t<double>>(const std::string&, const py::array_t<double>&, const py::object&);
template py::array_t<double> matrix_functions_wrapper<GretlFunIndex::F_CRIT, py::array_t<double>>(const std::string&, const py::array_t<double>&, const py::object&);
template py::array_t<double> matrix_functions_wrapper<GretlFunIndex::F_INVCDF, py::array_t<double>>(const std::string&, const py::array_t<double>&, const py::object&);
template py::array_t<double> matrix_functions_wrapper<GretlFunIndex::F_PDF, std::string>(const std::string&, const std::string&, const py::object&);
template py::array_t<double> matrix_functions_wrapper<GretlFunIndex::F_CDF, std::string>(const std::string&, const std::string&, const py::object&);
template py::array_t<double> matrix_functions_wrapper<GretlFunIndex::F_PVAL, std::string>(const std::string&, const std::string&, const py::object&);
template py::array_t<double> matrix_functions_wrapper<GretlFunIndex::F_CRIT, std::string>(const std::string&, const std::string&, const py::object&);
template py::array_t<double> matrix_functions_wrapper<GretlFunIndex::F_INVCDF, std::string>(const std::string&, const std::string&, const py::object&);

template <GretlFunIndex FI>
py::object matrix_cov_corr (const py::array &X, int dfcorr)
{
    //
    // here we perform matrix-based covariance/correlation analysis
    // this is an interface to gretl_covariance_matrix()
    //
    gretl_matrix_ptr mat{nullptr};      // gretl matrix we pass to libgretl
    gretl_matrix_ptr res_mat{nullptr};  // gretl matrix we get from libgretl
    NumPyArrayView arr_view;
    int err = 0;

    // we extract X matrix
    NumPyArray_extract(X, arr_view);
    if (arr_view.cols < 2) handle_gretl_error(E_TOOFEW);

    // we fill-up 'mat'
    mat.reset(gretl_matrix_alloc(arr_view.rows, arr_view.cols));
    NumPyArray_to_gretl_matrix<gretl_matrix>(arr_view, mat.get());

    // we do the job
    if constexpr (FI == GretlFunIndex::F_MCOV) {
        res_mat.reset(gretl_covariance_matrix(mat.get(), 0, dfcorr, &err));
    } else if constexpr (FI == GretlFunIndex::F_MCORR) {
        res_mat.reset(gretl_covariance_matrix(mat.get(), 1, 1, &err));
    } else {
        static_assert(always_false<GretlFunIndex>, "matrix_cov_corr: unknown GretlFunIndex value");
    }
    if (err) handle_gretl_error(err);

    auto res = convert_GretlType_to_GretlTypes(GRETL_TYPE_MATRIX,
                                               res_mat.get(), 0, err);
    if (err) handle_gretl_error(err);

    return variant_to_pyobject(std::move(*res));
}
template py::object matrix_cov_corr<GretlFunIndex::F_MCOV>(const py::array&, int);
template py::object matrix_cov_corr<GretlFunIndex::F_MCORR>(const py::array&, int);

void do_gretl_delete_variables (const std::string &varname, const py::kwargs &kwargs)
{
    //
    // function mimics libgretl's 'delete' command
    // note: not all options are available!!!
    //
    GretlDatasetPtr dataset = nullptr;
    gretl_list_ptr var_list{nullptr};
    std::string type_to_delete = "";    // type for bulk deletion
    std::vector<std::string> to_erase;  // variables to be erased after bulk deletion
    gretlopt opt = OPT_NONE;
    int renumber = 0;
    int err = 0;

    // we check if we don't deal with a package
    if (!varname.empty() && GretlPackage::package_loaded(varname)) throw std::invalid_argument("in gretl4py, the 'delete' function does not work with packages");

    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs_command(DELEET, dataset, kwargs);
        if (kwargs.contains("type")) type_to_delete = kwargs["type"].cast<std::string>();
    } else {
        // if we don't have kwargs, we must assign GD manually
        dataset = GretlDataset::default_dataset();
    }

    // we correctly deal with no arguments passed
    if (varname.empty() && !(opt & OPT_T)) handle_gretl_error(E_ARGS);

    // we look for variables in GD _only_ if there is no uservar
    if (!varname.empty() && get_user_var_by_name(varname.c_str()) == NULL && dataset) var_list = list_from_formula(varname, dataset->dset, err);

    // we do the job
    if (opt & OPT_T) {
        for (const auto &name : user_vars) {
            if (!err && (gretl_type_get_name(user_var_get_type_by_name(name.c_str())) == type_to_delete)) to_erase.push_back(name);
        }
        err = gretl_delete_variables(NULL, NULL, opt, dataset->dset, NULL, global_printer.prn_ptr());
    } else {
        if (!var_list && !dataset) throw std::invalid_argument("'" + varname + "': series does not exist");
        err = gretl_delete_variables(var_list.get(), var_list ? NULL : varname.c_str(), opt, dataset->dset,
                                     var_list ? &renumber : NULL, global_printer.prn_ptr());
    }
    global_printer.print();
    if (err) handle_gretl_error(err);

    // finally we erase uservar from registry
    if (opt & OPT_T) {
        for (const auto &name : to_erase) {
            user_vars.erase(name);
        }
    } else {
        if (user_vars.find(varname) != user_vars.end()) user_vars.erase(varname);
    }
}

template <GretlObjType OBJ_TYPE>
py::object do_get_accessor (const std::string &name, GretlDatasetPtr dataset, void *model)
{
    //
    // here we get accessor in the context of OBJ_TYPE, dataset, and model
    //
    int err = 0;
    auto res = gretl_generate_gretl_object(name.c_str(), err, OBJ_TYPE, dataset ? dataset->dset : NULL, model);

    if (err) handle_gretl_error(err);

    return variant_to_pyobject(std::move(*res));
}
template py::object do_get_accessor<GRETL_OBJ_ANY>(const std::string&, GretlDatasetPtr, void*);
template py::object do_get_accessor<GRETL_OBJ_DSET>(const std::string&, GretlDatasetPtr, void*);
template py::object do_get_accessor<GRETL_OBJ_EQN>(const std::string&, GretlDatasetPtr, void*);
template py::object do_get_accessor<GRETL_OBJ_VAR>(const std::string&, GretlDatasetPtr, void*);
template py::object do_get_accessor<GRETL_OBJ_SYS>(const std::string&, GretlDatasetPtr, void*);

// general interface to gretl's 'genr' command
void genr (const std::string &formula, GretlDatasetPtr data)
{
    // GretlType genr_get_last_output_type (void);
    gretl_generate<GRETL_TYPE_ANY>(formula, data ? data : GretlDataset::default_dataset());
}

// T = { py::array ; int ; double ; std::string }
template <typename T>
void genr (const std::string &name, const T &value)
{
    GretlType uv_type = GRETL_TYPE_NONE;
    void *uv_ptr = nullptr;
    int err = 0;

    if constexpr (std::is_same_v<T, py::array>) {
        uv_type = GRETL_TYPE_MATRIX;
        uv_ptr = static_cast<gretl_matrix*>(NULL);
        NumPyArrayView view;

        NumPyArray_extract(value, view);
        uv_ptr = gretl_matrix_alloc(view.rows, view.cols);
        if (uv_ptr == NULL) handle_gretl_error(E_ALLOC);
        NumPyArray_to_gretl_matrix<gretl_matrix>(view, static_cast<gretl_matrix*>(uv_ptr));
    } else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, int>) {
        uv_type = GRETL_TYPE_DOUBLE;
        uv_ptr = std::malloc(sizeof(double));
        if (uv_ptr == NULL) handle_gretl_error(E_ALLOC);
        *static_cast<double*>(uv_ptr) = static_cast<double>(value);
    } else if constexpr (std::is_same_v<T, std::string>) {
        uv_type = GRETL_TYPE_STRING;
        uv_ptr = std::malloc(value.size() + 1);
        if (uv_ptr == NULL) handle_gretl_error(E_ALLOC);
        std::strcpy(static_cast<char*>(uv_ptr), value.c_str());
    } else {
        static_assert(always_false<T>, "genr(): unknown type");
    }

    // we finally register variable
    err = user_var_add(name.c_str(), uv_type, uv_ptr);
    if (err) handle_gretl_error(err);
    user_vars.insert(name);
}
template void genr<py::array>(const std::string&, const py::array&);
template void genr<int>(const std::string&, const int&);
template void genr<double>(const std::string&, const double&);
template void genr<std::string>(const std::string&, const std::string&);

[[noreturn]] void handle_gretl_error (int err, const std::string &ext_msg, const std::string &ext_msg_desc)
{
    //
    // handle libgretl errors
    // ext_msg: additional information to be appended (e.g. file name)
    // ext_msg2: additional description for ext_msg
    // note: arg_errors is not complete!!!
    //
    static const std::unordered_set<int> arg_errors{ E_ARGS, E_INVARG, E_UNKVAR, E_TOOFEW };

    const char *msg_src = errmsg_get_with_default(err);
    std::string msg = msg_src ? msg_src : "unknown error";

    gretl_error_clear();

    if (err == E_FOPEN) throw file_not_found(msg + ": " + ext_msg);
    if (arg_errors.count(err)) {
        if (!ext_msg.empty()) msg.append(" for '").append(ext_msg).append("'");
        if (!ext_msg_desc.empty()) msg.append(": ").append(ext_msg_desc);
        throw std::invalid_argument(msg);
    }
    throw std::runtime_error(msg);
}

void gretl_valid_long_opt (gretlopt &opt, GretlCmdIndex ci, const std::string &key,
                           const py::handle &val, const char *ext_val)
{
    // should be a regular gretl option: check its validity in
    // the context of @ci
    // note: ext_val is to bypass kwargs and to force setting a given option
    gretlopt opt_local = OPT_NONE;
    OptStatus status;

    if (!is_pystr(val) && !is_pyint(val) && !is_pyfloat(val) && !is_pybool(val) && !ext_val) throw std::invalid_argument("unsupported option parameter type for " + key);

    if (std::strchr(key.c_str(), '_')) {
        std::string tmp(key);

        gretl_charsub(tmp.data(), '_', '-');
        opt_local = valid_long_opt(ci, tmp.c_str(), &status);
    } else {
        opt_local = valid_long_opt(ci, key.c_str(), &status);
    }
    if (opt_local == OPT_NONE) throw std::invalid_argument("invalid option flag: " + key);

    if (status == OPT_NO_PARM) {
        // no parameter wanted, but we'll accept a boolean
        if (!ext_val && !is_bool_kwarg(val)) return;
    } else {
        // an option parameter is accepted, perhaps required
        if (ext_val) {
            push_option_param(ci, opt_local, gretl_strdup(ext_val));
        } else {
            push_option_param(ci, opt_local, gretl_strdup(std::string(py::str(val)).c_str()));
        }
    }

    // finally transfer the option
    SET_OPT(opt, opt_local);
}

void unpack_kwargs_test (gretlopt &opt, const std::string &type,
                         const py::kwargs &kwargs, int &order)
{
    //
    // we setup test options
    // order: for autocorrelation or arch test (may change in future)
    //
    GretlCmdIndex ci = MODTEST;
    opt = OPT_NONE;

    // we set basic opt for given test
    if (type == "add") {
        ci = ADD;
    } else if (type == "adf") {
        ci = ADF;
    } else if (type == "arch") {
        opt = OPT_H;
    } else if (type == "autocorr") {
        opt = OPT_A;
    } else if (type == "bds") {
        ci = BDS;
    } else if (type == "bkw") {
        ci = BKW;
    } else if (type == "breusch-pagan") {
        opt = OPT_B;
    } else if (type == "chow") {
        ci = CHOW;
    } else if (type == "coeffsum") {
        ci = COEFFSUM;
    } else if (type == "coint") {
        ci = COINT;
    } else if (type == "comfac") {
        opt = OPT_C;
    } else if (type == "corrgm") {
        ci = CORRGM;
    } else if (type == "cusum") {
        ci = CUSUM;
    } else if (type == "fractint") {
        ci = FRACTINT;
    } else if (type == "difftest") {
        ci = DIFFTEST;
    } else if (type == "johansen") {
        ci = COINT2;
    } else if (type == "kpss") {
        ci = KPSS;
    } else if (type == "leverage") {
        ci = LEVERAGE;
    } else if (type == "levinlin") {
        ci = LEVINLIN;
    } else if (type == "logs") {
        opt = OPT_L;
    } else if (type == "meantest") {
        ci = MEANTEST;
    } else if (type == "normality") {
        opt = OPT_N;
    } else if (type == "normtest") {
        ci = NORMTEST;
    } else if (type == "omit") {
        ci = OMIT;
    } else if (type == "panel") {
        opt = OPT_P;
    } else if (type == "panspec") {
        ci = PANSPEC;
    } else if (type == "qlrtest") {
        ci = QLRTEST;
    } else if (type == "reset") {
        ci = RESET;
    } else if (type == "restrict") {
        ci = RESTRICT;
    } else if (type == "runs") {
        ci = RUNS;
    } else if (type == "squares") {
        opt = OPT_S;
    } else if (type == "white") {
        opt = OPT_W;
    } else if (type == "white-nocross") {
        opt = OPT_X;
    } else if (type == "vartest") {
        ci = VARTEST;
    } else if (type == "vif") {
        ci = VIF;
    } else if (type == "xdepend") {
        opt = OPT_D;
    } else {
        throw std::invalid_argument("unknown test");
    }

    // now we loop over kwargs;
    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) throw std::invalid_argument("option key must be a string");
        const std::string &key = item.first.cast<std::string>();

        if (key == "order") {
            if (!is_pyint(item.second)) throw std::invalid_argument("'order' takes integers only");
            order = item.second.cast<int>();
            continue;
        } else if (key == "order_m" && type == "levinlin") {
            // special case: we must do something with it later
            continue;
        } else if (key == "silent") {
            // special case: silent is processed manually later on
            continue;
        } else if (key == "plot") {
            throw std::invalid_argument("invalid option flag: " + key);
        } else {
            gretl_valid_long_opt(opt, ci, key, item.second);
        }
    }

    // we force --plot=none for tests
    if (type == "cusum" || type == "leverage" || type == "qlrtest" || type == "corrgm") {
        SET_OPT(opt, OPT_U);
        gretl_valid_long_opt(opt, ci, "plot", py::handle(), "none");
    }
}

gretl_list_ptr list_from_formula (const std::string &formula, DATASET *dset, int &err, bool handle_error)
{
    //
    // we build libgretl's list based on provided formula
    // note: if handle_error == false, we do not handle error
    //
    std::string tmp = formula;

    // convert from a simple R-style formula to a gretl list
    std::replace(tmp.begin(), tmp.end(), '~', ' ');
    std::replace(tmp.begin(), tmp.end(), '+', ' ');

    // we do the job
    int *raw_list = generate_list(tmp.c_str(), dset, 0, &err);

    if (handle_error && err) handle_gretl_error(err);

    return gretl_list_ptr(raw_list);
}

GretlTypesPtr gretl_generate_gretl_object (const char *s, int &err, GretlObjType obj_type, DATASET *dset, void *model)
{
    //
    // we grab object from libgretl stack
    //
    GretlTypesPtr res;                      // gretl4py type
    GretlType t = GRETL_TYPE_NONE;          // libgretl type
    gretl_dataset_ptr dset_ptr{nullptr};    // used when dset == NULL
    void *resp;

    // we allocate dset if we didn't get one
    if (dset == NULL) {
        dset_ptr.reset(datainfo_new());
        dset = dset_ptr.get();
    }

    // we grab object from stack (note: if model was passed we use it)
    if (model != NULL) set_genr_model(model, obj_type);
    resp = generate_gretl_object(s, dset, &t, &err);
    if (model != NULL) unset_genr_model();
    if (!err) res = convert_GretlType_to_GretlTypes(t, resp, dset->n, err);
    if (err) handle_gretl_error(err);

    return res;
}

template <GretlType GR_TYPE>
void gretl_generate (const std::string &formula, GretlDatasetPtr dataset, const std::string &name)
{
    //
    // General interface to libgretl's generate()
    // create a gretl object of GR_TYPE according to @name and @formula
    // in the context of @GD
    // Note: if 'name' is empty, 'formula' must contain the full definition
    //
    std::string varname;
    int err = 0;

    if (!dataset) dataset = GretlDataset::default_dataset();
    if constexpr (GR_TYPE == GRETL_TYPE_LIST || GR_TYPE == GRETL_TYPE_SERIES) {
        if (!dataset) handle_gretl_error(E_NODATA);
    }

    // determine whether to use an explicit name or parse it from formula
    if (name.empty()) {
        err = generate(formula.c_str(), dataset ? dataset->dset : NULL, GR_TYPE, OPT_NONE, global_printer.prn_ptr());
        if constexpr (GR_TYPE != GRETL_TYPE_SERIES) varname = extract_varname(formula);
    } else {
        const std::string expr = name + "=" + formula;

        err = generate(expr.c_str(), dataset ? dataset->dset : NULL, GR_TYPE, OPT_NONE, global_printer.prn_ptr());
        if constexpr (GR_TYPE != GRETL_TYPE_SERIES) varname = name;
    }
    if (err) handle_gretl_error(err);

    // register user-defined variable name (if applicable)
    if constexpr (GR_TYPE != GRETL_TYPE_SERIES) {
        if (!varname.empty()) user_vars.insert(varname);
    }

    global_printer.print();
}
template void gretl_generate<GRETL_TYPE_ANY>(const std::string&, GretlDatasetPtr, const std::string&);
template void gretl_generate<GRETL_TYPE_LIST>(const std::string&, GretlDatasetPtr, const std::string&);
template void gretl_generate<GRETL_TYPE_SERIES>(const std::string&, GretlDatasetPtr, const std::string&);

template <typename T>
void gretl_execute_script (const T &script)
{
    //
    // apparatus for executing regular hansl code
    //
    char line[MAXLINE] = {'\0'};
    MODEL *model = NULL;
    DATASET *dset = NULL;
    ExecState state;
    CMD cmd;
    int err = 0;

    // lambda for clearing a dataset
    auto clear_dataset = [&dset](void) -> void
    {
        script_dataset.reset(datainfo_new());
        dset = script_dataset.get();
    };

    // lambda for executing just one line
    auto exec_line = [&state, &dset, clear_dataset](const std::string &line) -> void
    {
        int err = 0;
        CMD *cmd = NULL;

        *state.line = '\0';
        std::strncat(state.line, line.c_str(), MAXLINE-2);
        std::strcat(state.line, "\n");
        err = parse_command_line(&state, dset, NULL);
        if (err) handle_gretl_error(err);
        cmd = state.cmd;
        if (cmd->ci == OPEN && nulldata_executed) {
            clear_dataset();
            nulldata_executed = false;
        }
        if (cmd->ci == NULLDATA) {
            if (cmd->order < 1) handle_gretl_error(E_INVARG, "nulldata", "data series length count missing or invalid");
            // in gretlcli.c, cli_clear_data() is called here => lines below taken from there
            clear_dataset();
            clear_model(state.model);
            gretl_cmd_destroy_context(cmd);
            err = open_nulldata(dset, 1, cmd->order, cmd->opt, global_printer.prn_ptr());
            if (!err && !nulldata_executed) nulldata_executed = true;
        } else {
            err = gretl_cmd_exec(&state, dset);
        }
        if (err) handle_gretl_error(err);
        global_printer.print();
    };

    // dataset and model to use
    model = allocate_working_model();
    if (model == NULL) handle_gretl_error(E_ALLOC);
    if (!script_dataset && !nulldata_executed) script_dataset.reset(datainfo_new());
    dset = script_dataset.get();
    if (dset == NULL) handle_gretl_error(E_ALLOC);

    // init apparatus
    err = gretl_cmd_init(&cmd);
    if (err) handle_gretl_error(err);
    gretl_exec_state_init(&state, CONSOLE_EXEC, line, &cmd, model, global_printer.prn_ptr());

    // do the job
    if constexpr (std::is_same_v<T, std::string>) {
        exec_line (script);
    } else if constexpr (std::is_same_v<T, py::iterable>) {
        for (const auto &el : script) {
            if (!is_pystr(el)) handle_gretl_error(E_INVARG, py::str(el));
            exec_line(el.template cast<std::string>());
        }
    } else {
        static_assert(always_false<T>, "gretl_execute_script(): unknown type");
    }
    gretl_object_unref(model, GRETL_OBJ_EQN);
}
template void gretl_execute_script<std::string>(const std::string&);
template void gretl_execute_script<py::iterable>(const py::iterable&);
