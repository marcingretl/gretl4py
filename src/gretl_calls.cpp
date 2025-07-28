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
#include <gretl/gretl_errors.h>
#include <gretl/libset.h>
extern "C" {
    #include <gretl/gretl_string_table.h>
    #include <gretl/gretl_typemap.h>
}
#include <iostream>
#include <cstdlib>
#include <unordered_set>

#if defined(_WIN32)
# include <gretl/gretl_win32.h>
#endif

#if (defined __GNUC__) && !(defined __clang__)
# define COMPILER_IDENT "GCC " __VERSION__
#else
# define COMPILER_IDENT __VERSION__
#endif

#define LIBGRETL_MIN_VER 51

#define GRETL_PYBIND11_DEBUG 0

// global variables (some used outside this file)
std::shared_ptr<GretlDataset> DSET = nullptr;           // default dataset
std::unordered_set<std::string> UserVars;               // registry of user variables
std::map<std::string, GretlPackagePtr> GPKGs;           // registry of loaded contributed packages
static GretlPrint GP = GretlPrint();                    // printer
static int gretl4py_verbose = 0;

//////////////////////////////////////////////////
// module related tools
//////////////////////////////////////////////////

static std::string gretl4py_logo (void)
{
    std::string logo = "";

    logo +="                _   _   ___\n";
    logo +="               | | | | /   |\n";
    logo +="  __ _ _ __ ___| |_| |/ /| |_ __  _   _\n";
    logo +=" / _` | '__/ _ \\ __| / /_| | '_ \\| | | |\n";
    logo +="| (_| | | |  __/ |_| \\___  | |_) | |_| |\n";
    logo +=" \\__, |_|  \\___|\\__|_|   |_/ .__/ \\__, |\n";
    logo +="  __/ |                    | |     __/ |\n";
    logo +=" |___/                     |_|    |___/\n";

    return logo;
}

void module_init (void)
{
    // check libgretl version
    if (libgretl_version() < LIBGRETL_MIN_VER) {
        throw std::runtime_error("gretl4py requires libgretl-1.0 version "
                                 + std::to_string(LIBGRETL_MIN_VER) + " or higher");
    }

    // initialize libgretl
    libgretl_init();

    // we set user workdir, gretldir and dotdir
    py::module_ pathlib = py::module_::import("pathlib");
    py::object start = pathlib.attr("Path").attr("cwd")();
    std::string start_dir = py::str(start);

    if (int err = gretl_set_path_by_name("workdir", start_dir.c_str())) {
        handle_gretl_error(err);
    }
    if (const char* env = std::getenv("GRETL_HOME"); env) {
        if (int err = gretl_set_path_by_name("gretldir", env)) {
            handle_gretl_error(err);
        }
#if defined(_WIN32)
        std::string dotdir_path = std::string(env).append("\\tmp");
#else
        std::string dotdir_path = std::string(env).append("/tmp");
#endif // defined
        if (int err = gretl_set_path_by_name("dotdir", dotdir_path.c_str())) {
            handle_gretl_error(err);
        }
    }

    // read and set GRETL_PLUGIN_PATH if available
    if (const char* env = std::getenv("GRETL_PLUGIN_PATH"); env) {
        set_gretl_plugin_path(env);
    }

    // read and set GRETL4PY_VERBOSE if available
    if (const char* env = std::getenv("GRETL4PY_VERBOSE"); env) {
        gretl4py_verbose = std::atoi(env);
    }

    // verbose output
    if (gretl4py_verbose) {
        std::printf("libgretl initialized\n");
    }
}

void module_cleanup (void)
{
    int err = 0;

    for (const auto &name : UserVars) {
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

    libgretl_cleanup();

    if (gretl4py_verbose) {
        std::printf("libgretl cleaned up\n");
    }
}

py::dict about (bool do_print)
{
    GretlBundle b = GretlBundle();
    const std::string lgretl_strings[9] =
        {"dotdir", "gnuplot", "gretldir", "seats", "tramo",
         "tramodir", "workdir", "x12a", "x12adir"};
    std::string mod_ver, mod_logo, lg_ver, py11_ver, sj_ver, sj_name, sj_desc;
    py::dict res, built_in_strings;

    gretl_push_c_numeric_locale();

    mod_ver = std::to_string(GRETL4PY_VERSION).substr(0,4);
    mod_logo = gretl4py_logo();
    py11_ver = std::to_string(PYBIND11_VERSION_MAJOR);
    py11_ver += "." + std::to_string(PYBIND11_VERSION_MINOR);
    py11_ver += "." + std::to_string(PYBIND11_VERSION_PATCH);
    lg_ver = std::to_string(libgretl_version());

    // built-in strings
    for (int i=0; i<9; i++ ) {
        built_in_strings[py::cast(lgretl_strings[i])] =
            get_built_in_string_by_name(lgretl_strings[i].c_str());
    }

    res[py::cast("mod_ver")] = mod_ver;
    res[py::cast("mod_logo")] = mod_logo;
    res[py::cast("py11_ver")] = py11_ver;
    res[py::cast("lg_ver")] = lg_ver;
    if (!b.err) {
        res[py::cast("sysinfo")] = b.to_dict();
    } else {
        res[py::cast("sysinfo")] = py::none();
    }
    res[py::cast("built_in_strings")] = built_in_strings;

    if (do_print || gretl4py_verbose) {
        std::cout << mod_logo << std::endl;
        std::cout << "gretl4py v" << mod_ver;
        std::cout << " [" << COMPILER_IDENT << "]" << std::endl;
        std::cout << "\t+ pybind11     v" << py11_ver << std::endl;
        std::cout << "\t+ libgretl-1.0 v" << lg_ver << std::endl;
        if (!b.err) {
            std::cout << std::endl << "libgretl $sysinfo:" << std::endl;
            b.print();
        }
        std::cout << "libgretl built-in strings:" << std::endl;
        for (auto item : built_in_strings) {
            std::cout << "  " << item.first << " = ";
            if (!MapPyToCpp(item.second).to_string().empty()) {
                std::cout << "\"" << item.second << "\"";
            }
            std::cout << std::endl;
        }
        py::print("\ngretl_plugin_path = \"" + std::string(gretl_plugin_path()) + "\"");
    }

    gretl_pop_c_numeric_locale();

    if (b.err) {
        std::cerr << errmsg_get_with_default(b.err) << std::endl;
    }

    return res;
}

void do_set (const std::string key,
             const std::optional<py::object> &pyval)
{
    //
    // interface to libgretl's set
    //
    int err = 0;

    if (pyval == std::nullopt) {
        err = execute_set(key.c_str(), NULL, NULL, OPT_NONE, GP.prn_ptr());
    } else {
        err = execute_set(key.c_str(), MapPyToCpp(pyval).to_string().c_str(),
                          NULL, OPT_NONE, GP.prn_ptr());
    }
    if (err) {
        handle_gretl_error(err);
    }

    GP.print();
}

//////////////////////////////////////////////////
// static functions
//////////////////////////////////////////////////

static gretlopt unpack_kwargs_command (GretlCmdIndex ci,
                                       std::shared_ptr<GretlDataset> &GD,
                                       const py::kwargs &kwargs)
{
    //
    // unpack kwargs for libgretl's command interfaces
    //
    gretlopt opt = OPT_NONE;

    if (len(kwargs) > 0) {
        for (auto &item : kwargs) {
            if (!is_pystr(item.first)) {
                throw std::invalid_argument("option key must be a string");
            }
            const std::string &key = item.first.cast<std::string>();

            if (key == "data") {
                // special case: we use already allocated object of GretlDataset class
                GD = item.second.cast<std::shared_ptr<GretlDataset>>();
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

        // we force --plot=none for certain commands
        if (ci == CORRGM) {
            opt = static_cast<gretlopt>(opt | OPT_U);
            gretl_valid_long_opt(opt, ci, "plot", py::handle(), "none");
        }
    }

    // if we didn't get dataset, let use global
    if (!GD) {
        GD = DSET;
    }

    return opt;
}

//////////////////////////////////////////////////
// data management
//////////////////////////////////////////////////

void do_summary (const std::string &varlist, const py::kwargs &kwargs)
{
    //
    // this function mimics libgretl's 'summary' command
    //
    std::shared_ptr<GretlDataset> GD = nullptr;
    gretl_list_ptr varlist_ptr{nullptr};
    int err = 0;

    // we unpack kwargs at first
    // Note: GD is set here
    gretlopt opt = unpack_kwargs_command(SUMMARY, GD, kwargs);
    if (!GD) {
        throw std::runtime_error("no dataset available");
    }

    // be build gretl_list
    if (!varlist.empty()) {
        varlist_ptr = list_from_formula(varlist, GD->dset, err);
    }
    if (err) {
        handle_gretl_error(err);
    }

    // this code was taken from 'interact.c'
    err = incompatible_options(opt, static_cast<gretlopt>(OPT_B | OPT_W));
    if (err) {
        handle_gretl_error(err);
    }
    if (opt & OPT_B) {
        err = summary_statistics_by(varlist_ptr.get(), GD->dset, opt, GP.prn_ptr());
    } else if (opt & OPT_X) {
        err = matrix_command_driver(SUMMARY, varlist_ptr.get(),
                                    kwargs["matrix"].cast<std::string>().c_str(),
                                    GD->dset, opt, GP.prn_ptr());
    } else {
        err = list_summary_driver(varlist_ptr.get(), GD->dset, opt, GP.prn_ptr());
    }
    if (err) {
        handle_gretl_error(err);
    }

    GP.print();
}

py::object load_package (const std::string &name)
{
    //
    // this function mimics libgretl's 'include' command
    // for now it acts as factory function for GretlPackage class
    //
    std::string basename;
    std::string suffix = "";
    GretlPackagePtr pkg = nullptr;

    // we check if we deal with suffix-ed string or just basename
    if (has_suffix(name.c_str(), ".gfn") || has_suffix(name.c_str(), ".zip")) {
        size_t dot_pos = name.find('.');

        basename = name.substr(0, dot_pos);
        suffix = name.substr(dot_pos);
    } else {
        basename = name;
    }

    // we look up for package in registry
    auto it = GPKGs.find(basename);
    if (it != GPKGs.end()) {
        pkg = it->second;
    } else {
        pkg = std::make_shared<GretlPackage>(basename, suffix);
        GPKGs.emplace(basename, pkg);
    }

    // get list of exported function names
    py::object obj = py::cast(pkg);
    py::object func_list = obj.attr("_get_functions")();

    // import the 'gretl' module dynamically
    py::module_ gretl_mod = py::module_::import("gretl");

    // for each function, attach a Python callable:
    // pkg.fname = lambda **kwargs: ::_function_exec(fname, **kwargs)
    for (const py::handle &py_fname : func_list) {
        py::str fname = py::str(py_fname);

        // create a wrapper bound to this function name and obj
        py::object wrapper = py::cpp_function (
            [obj, fname](const std::string &output, const py::kwargs &kwargs) -> py::object
            {
                return obj.attr("_function_exec")(fname, output, **kwargs);
            }, py::arg("output").none(true) = "", py::kw_only());

        // set it as an attribute on the object
        //obj.attr(fname) = wrapper;
        gretl_mod.attr(fname) = wrapper;
    }

    return obj;
}

// stuff below is volatile
void gretl_user_var_delete_by_name (const std::string &name)
{
    if (UserVars.find(name) != UserVars.end()) {
        int err = user_var_delete_by_name(name.c_str(), GP.prn_ptr());
        if (err) {
            handle_gretl_error(err);
        }
        UserVars.erase(name);
    } else {
        throw std::invalid_argument("no such variable: '" + name + "'");
    }
}

void gretl_print_user_var_by_name (const std::string &name,
                                   std::shared_ptr<GretlDataset> data)
{
    if (UserVars.find(name) == UserVars.end()) {
        throw std::invalid_argument("unknown variable: '" + name + "'");
    }

    DATASET *dset = NULL;

    if (data) {
        dset = data->dset;
    } else if (DSET) {
        dset = DSET->dset;
    }

    int err = print_user_var_by_name(name.c_str(), dset, OPT_NONE, GP.prn_ptr());
    if (err) {
        handle_gretl_error(err);
    }

    GP.print();
}

void uservars_list (void)
{
    std::string var_type_name;

    py::print("Registered variables:");
    for (const auto &name : UserVars) {
        var_type_name = gretl_type_get_name(user_var_get_type_by_name(name.c_str()));

        py::print(name, "(" + var_type_name + ")");
    }
    py::print("\n");
}

// general inteface to gretl's 'genr' command
void genr (const std::string &formula, std::shared_ptr<GretlDataset> data)
{
    // GretlType genr_get_last_output_type (void);
    gretl_generate<GRETL_TYPE_ANY>(formula, data ? data : DSET);
}

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
        if (uv_ptr == NULL) {
            handle_gretl_error(E_ALLOC);
        }
        NumPyArray_to_gretl_matrix<gretl_matrix>(view, static_cast<gretl_matrix*>(uv_ptr));
    } else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, int>) {
        uv_type = GRETL_TYPE_DOUBLE;
        uv_ptr = std::malloc(sizeof(double));
        if (uv_ptr == NULL) {
            handle_gretl_error(E_ALLOC);
        }
        *static_cast<double*>(uv_ptr) = static_cast<double>(value);
    } else if constexpr (std::is_same_v<T, std::string>) {
        uv_type = GRETL_TYPE_STRING;
        uv_ptr = std::malloc(value.size() + 1);
        if (uv_ptr == NULL) {
            handle_gretl_error(E_ALLOC);
        }
        std::strcpy(static_cast<char*>(uv_ptr), value.c_str());
    } else {
        throw std::runtime_error("unsupported type");
    }

    // we finally register variable
    err = user_var_add(name.c_str(), uv_type, uv_ptr);
    if (err) {
        handle_gretl_error(err);
    }
    UserVars.insert(name);
}
template void genr<py::array>(const std::string&, const py::array&);
template void genr<int>(const std::string&, const int&);
template void genr<double>(const std::string&, const double&);
template void genr<std::string>(const std::string&, const std::string&);
