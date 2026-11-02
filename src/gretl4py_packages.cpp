/*
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  gretl4py -- Python bindings for gretl
 *  Copyright (C) 2024 Marcin Błażejowski
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

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "gretl4py_types.h"
#include "gretl4py_packages.h"
#include "gretl4py_utils.h"
#include "gretl4py_run_hansl.h"
#include <gretl/gretl_matrix.h>
extern "C" {
    #include <gretl/uservar.h>
    #include <gretl/gretl_func.h>
    #include <gretl/gretl_typemap.h>
}
#include <iomanip>
#include <cstdio>

extern std::unordered_set<std::string> user_vars;

//
// GretlPackage class methods
//
GretlPackage::GretlPackage (const std::string &pkg_name, const std::string &suffix, bool print_summary) : pkg_name_(pkg_name), suffix_(suffix)
{
    // lambda for sorting function arguments by id-s
    auto sort_func_args_by_id = [](const std::map<std::string, int> &func_args) -> FunctionArgsVec
    {
        // vector of func_name_args items ordered by value (int)
        FunctionArgsVec func_args_sorted(func_args.begin(), func_args.end());

        std::sort(func_args_sorted.begin(), func_args_sorted.end(),
                  [](const auto &a, const auto &b)
                  {
                      return a.second < b.second; // sort by value
                  });

        return func_args_sorted;
    };

    // lambda for function metadata registration
    auto register_function_metadata = [this](const char *func_name) -> void
    {
        // pointers
        auto func_ptr = get_function_from_package(func_name, pkg_);

        if (!func_ptr) throw std::runtime_error("unable to get pointer to function: " + std::string(func_name));
        func_ptrs_.emplace(func_name, std::move(func_ptr));
        // arguments
        std::map<std::string, int> arg_names_tmp;

        for (int i=0; i<fn_n_params(func_ptr); ++i) {
            arg_names_tmp.emplace(fn_param_name(func_ptr, i), i);
        }
        func_args_.emplace(func_name, std::move(arg_names_tmp));
    };

    static const std::unordered_set<UfunRole> excluded_roles = {
        UFUN_GUI_MAIN, UFUN_GUI_PRECHECK, UFUN_UI_MAKER
    };//UFUN_BUNDLE_PLOT, UFUN_PLOT_PRECHECK
    std::unordered_set<std::string> excluded_functions;
    std::ostringstream oss;
    char path[FILENAME_MAX];    // get_full_filename() requires a buffer of at least FILENAME_MAX bytes
    char *raw_ptr = nullptr;    // placeholder for PDF filename
    int idxp = 0;

    // we try to find a path to the package
    *path = '\0';
    if (suffix.empty()) {
        err_ = get_full_filename((pkg_name + ".gfn").c_str(), path, OPT_I);
    } else {
        err_ = get_full_filename((pkg_name + suffix).c_str(), path, OPT_I);
    }
    if (err_) handle_gretl_error(err_, pkg_name);

    // we load the package
    pkg_ = get_function_package_by_filename(path, &err_);
    if (err_) handle_gretl_error(err_);
    if (pkg_name != function_package_get_name(pkg_)) throw std::runtime_error("package name mismatch: internal: '"
        + std::string(function_package_get_name(pkg_)) + "', provided: '" + pkg_name_ + "'");
    full_path_ = path;

    // we setup parent dir
    // Note: we cannot use .parent_path() here due to some differences
    auto pos = full_path_.find_last_of("/\\");
    if (pos != std::string::npos) {
        base_dir_ = full_path_.substr(0, pos);
    } else {
        base_dir_ = "";
    }

    // setup some basic info about package
    err_ = bundle_function_package_info(full_path_.c_str(), gb_.get_ptr());
    if (err_) handle_gretl_error(err_);
    version_ = std::get<double>(*gb_.get_element<GretlTypesPtr>("version"));
    long_version_ = get_properties_<std::string>("version");

    // we initialize package functions
    function_names_init();

    // we grab only public functions (names and indexes)
    // Note: we exclude functions with specific roles (mostly related with plotting and GUI)
    for (auto role : excluded_roles) {
        auto func_name = get_function_with_role(role);

        if (!func_name.empty()) excluded_functions.insert(std::move(func_name));
    }

    // we register functions metadata
    while (const char *func_name = next_available_function_name(pkg_, &idxp)) {
        if (gretl_is_public_user_function(func_name) && !excluded_functions.count(func_name)) register_function_metadata(func_name);
    }
    // workaround for packages in which the _only_ one public
    // function is marked as 'UFUN_GUI_MAIN' (TVC, for example)
    if (func_ptrs_.empty()) register_function_metadata(get_function_with_role(UFUN_GUI_MAIN).c_str());
    if (func_ptrs_.empty()) throw std::runtime_error("no public functions found in '" + pkg_name + "' package");

    // we construct function map with sorted arguments
    for (const auto &[key, el] : func_args_) func_args_sorted_.emplace(key, sort_func_args_by_id(el));

    // we grab name of PDF info
    if (function_package_has_PDF_doc(pkg_, &raw_ptr)) pdfname_.reset(raw_ptr);

    // package summary
    if (print_summary) {
        py::print(pkg_name_ + " " + long_version_ + ", " + std::get<std::string>(*gb_.get_element<GretlTypesPtr>("date"))
                  + " (" + get_properties_<std::string>("author") + ")" + ", JEL codes: " + get_properties_<std::string>("tags") );
        py::print();
    }
}

GretlPackage::~GretlPackage (void)
{
    err_ = function_package_unload_full(pkg_name_.c_str());
    if (err_) std::cerr << errmsg_get_with_default(err_) << std::endl;
}

void GretlPackage::clear_registry (void)
{
    //
    // we clear the whole packages registry
    //
    gpkgs_.clear();
}

GretlPackagePtr GretlPackage::load_package (const std::string &pkgname, const std::string &suffix, bool print_summary)
{
    //
    // here we register contributed package
    // at first, we look up for package in registry
    //
    // at first we check if package already exists
    auto it = gpkgs_.find(pkgname);

    if (it != gpkgs_.end()) return it->second;

    // now we load the package
    GretlPackagePtr pkg(new GretlPackage(pkgname, suffix, print_summary));
    gpkgs_.emplace(pkgname, pkg);

    return pkg;
}

bool GretlPackage::package_loaded (const std::string &pkgname)
{
    //
    // here we check if give package exists in registry
    //
    return gpkgs_.count(pkgname) == 0 ? false : true;
}

// T = { DataReq ; std::vector<std::string> ; std::string }
template <typename T>
T GretlPackage::get_properties_ (const char *property)
{
    //
    // we get package properties
    //
    T res;
    gchar *val_raw = nullptr;
    gretl_gchar_ptr val{nullptr};

    if constexpr (std::is_same_v<T, DataReq>) {
        err_ = function_package_get_properties(pkg_, property, &res, nullptr);
    } else {
        err_ = function_package_get_properties(pkg_, property, &val_raw, nullptr);
        val.reset(val_raw);
    }
    if (err_) handle_gretl_error(err_);

    // we produce output
    if constexpr (std::is_same_v<T, DataReq>) {
        return res;
    } else if (!val) {
        return {}; // gracefully return empty value
    } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
        load_string_to_container<std::vector<std::string>>(res, val.get());
    } else if constexpr (std::is_same_v<T, std::string>) {
        res = val.get();
    } else {
        static_assert(always_false<T>, "GretlPackage::get_properties_(): unknown type");
    }

    return res;
}

void GretlPackage::unpack_kwargs_package_ (const std::string &func_name, fncall *func_call, GretlDatasetPtr &dataset, const py::kwargs &kwargs)
{
    //
    // here we extract kwargs for any package function
    // note: at first we look for general (not function-related) options
    // next we push function arguments on stack keeping
    // original function order, so thanks to that function arguments
    // may be passed in kwargs in arbitrary order
    //
    if (py::len(kwargs) == 0) return;

    // map function to call with its arguments
    FunctionArgsVec func_name_args = get_func_args(func_name);

    // general options (non function-based)
    for (const auto &[key, val] : kwargs) {
        auto key_str = py::cast<std::string>(key);

        if (key_str == "data") {
            // we use GretlDataset object passed on function call
            dataset = val.cast<GretlDatasetPtr>();
            continue;
        } else if (key_str == "quiet" || key_str == "silent") {
            // do something elsewhere
            continue;
        } else {
            if (!std::any_of(func_name_args.begin(), func_name_args.end(), [&](const auto &p){ return p.first == key_str; }))
                throw std::invalid_argument(key_str + ": there is no such argument in function: " + func_name);
        }
    }

    // function-related options (arguments pushed on stack)
    std::string arg_name;   // variable to be pushed on stack (name)
    void *arg_value;        // pointer to variable's value to be pushed on stack
    GretlType arg_type;     // variable's type
    double darg_value;      // value to be pushed on stack (if double)

    for (const auto &[key, arg_no] : func_name_args) {
        if (!kwargs.contains(key)) {
            // if parameter is [optional] and _does not_ have [default] (i.e. does not have default value), we must set it to GRETL_TYPE_NONE
            if (fn_param_optional(func_get_ptr(func_name), arg_no) && !fn_param_has_default(func_get_ptr(func_name), arg_no)) {
                err_ = push_function_arg(func_call, key.c_str(), nullptr, GRETL_TYPE_NONE, nullptr);
                if (err_) handle_gretl_error(err_);
            }
            continue;
        }
        auto val = kwargs[py::cast(key)];
        PythonArgInfo arginfo = inspect_pyobject(val);
        arg_type = GRETL_TYPE_NONE;
        arg_value = nullptr;
        auto key_type = static_cast<GretlType>(fn_param_type(func_get_ptr(func_name), arg_no));

        if (key_type == GRETL_TYPE_SERIES && dataset ) {
            // special case: for series we swap situation and
            // evaluate arg via type declared in a function
            arg_name = py::cast<std::string>(val);
            int vnum = dataset->get_series_id(arg_name);

            err_ = push_function_arg(func_call, arg_name.c_str(), nullptr, GRETL_TYPE_USERIES, &vnum);
            if (err_) handle_gretl_error(err_);
            continue;
        } else if (arginfo.gtype == GRETL_TYPE_BUNDLE) {
            // note: here we deal with both: GRETL_TYPE_BUNDLE and GRETL_TYPE_BUNDLE_REF
            // final distinction is made by key_type
            auto gb = val.cast<GretlBundlePtr>();

            err_ = push_function_arg(func_call, nullptr, nullptr, key_type, gb->get_ptr());
            if (err_) handle_gretl_error(err_);
            continue;
        } else if (arginfo.gtype == GRETL_TYPE_MATRIX) {
            // note: here we deal with both: GRETL_TYPE_MATRIX and GRETL_TYPE_MATRIX_REF
            // final distinction is made by key_type
            if (arginfo.pytype == "GretlMatrix") {
                auto gm = val.cast<GretlMatrixPtr>();

                err_ = push_function_arg(func_call, nullptr, nullptr, key_type, gm->get_ptr());
            } else {
                if (key_type == GRETL_TYPE_MATRIX_REF) handle_gretl_error(E_INVARG, key.c_str(), "must be GretlMatrix, not NumPy");
                NumPyArrayView arr_view;
                gretl_matrix_ptr mat{nullptr};
                auto arr = val.cast<py::array>();

                NumPyArray_extract(arr, arr_view);
                if (!arr_view.data) throw std::runtime_error("NumPy array: null pointer.");
                mat.reset(gretl_matrix_alloc(arr_view.rows, arr_view.cols));
                if (!mat) handle_gretl_error(E_ALLOC);
                NumPyArray_to_gretl_storage<gretl_matrix>(arr_view, mat.get());
                err_ = push_function_arg(func_call, key.c_str(), nullptr, GRETL_TYPE_MATRIX, mat.release());
            }
            if (err_) handle_gretl_error(err_);
            continue;
        } else if (arginfo.gtype == GRETL_TYPE_LIST) {
            if (!dataset) handle_gretl_error(E_NODATA);
            auto list = dataset->gretllist(val.cast<py::list>());

            err_ = push_function_arg(func_call, key.c_str(), nullptr, GRETL_TYPE_LIST, list.release());
            if (err_) handle_gretl_error(err_);
            continue;
        } else if (arginfo.gtype == GRETL_TYPE_STRING) {
            // special case: we need to know if we deal with
            // user defined variable uv, one-element GRETL_TYPE_LIST or string per se
            arg_name = py::cast<std::string>(val);
            bool is_ref_user_var = false;

            // here we check if we deal with reference
            if (arg_name.substr(0, 1) == "&") {
                is_ref_user_var = true;
                arg_name = arg_name.substr(1);
            }
            if (user_vars.find(arg_name) != user_vars.end()) {
                user_var *uv = get_user_var_by_name(arg_name.c_str());

                // for reference, we change uv type!!!
                arg_type = is_ref_user_var ? gretl_type_get_ref_type(user_var_get_type(uv)) : user_var_get_type(uv);
                err_ = push_function_arg(func_call, arg_name.c_str(), uv, arg_type, user_var_get_value(uv));
                if (err_) handle_gretl_error(err_);
                continue;
            } else if (key_type == GRETL_TYPE_LIST) {
                if (!dataset) handle_gretl_error(E_NODATA);
                auto list = dataset->gretllist(val.cast<std::string>());

                err_ = push_function_arg(func_call, key.c_str(), nullptr, GRETL_TYPE_LIST, list.release());
                if (err_) handle_gretl_error(err_);
                continue;
            } else {
                char *arg_name_copy = strdup(arg_name.c_str());

                arg_value = static_cast<void*>(arg_name_copy);
                arg_type = arginfo.gtype;
            }
        } else if (arginfo.gtype == GRETL_TYPE_BOOL || arginfo.gtype == GRETL_TYPE_DOUBLE || arginfo.gtype == GRETL_TYPE_INT) {
            darg_value = py::cast<double>(val);
            arg_value = &darg_value;
            arg_type = GRETL_TYPE_DOUBLE;
        } else {
            throw std::runtime_error("variable " + key + ": unsupported type: " + arginfo.pytype);
        }
        err_ = push_function_arg(func_call, key.c_str(), nullptr, arg_type, arg_value);
        if (err_) handle_gretl_error(err_);
    }
}

int GretlPackage::get_err (void) const
{
    return err_;
}

std::string GretlPackage::repr_impl (void)
{
    err_ = print_function_package_info(full_path_.c_str(), 0, printer_.prn_ptr());
    if (err_) handle_gretl_error(err_);

    return printer_.get_buffer();
}

py::list GretlPackage::get_help (void)
{
    //
    // return either text help or path to pdf
    //
    py::list res;

    if (pdfname_) {
        std::filesystem::path pdf_path = std::filesystem::path(base_dir_) / pdfname_.get();

        res.append("pdf");
        res.append(pdf_path.string());
    } else {
        res.append("txt");
        err_ = print_function_package_help(full_path_.c_str(), nullptr, printer_.prn_ptr());
        if (err_) handle_gretl_error(err_);
        res.append(printer_.get_buffer());
    }

    return res;
}

std::string GretlPackage::get_name (void) const
{
    return pkg_name_;
}

//use_lgretl: if we use gretl4py code (true) or libgretl (false)
template <bool use_lgretl>
void GretlPackage::show_functions (const std::string &name)
{
    //
    // prints the signatures of all public functions provided by the package
    // in @name is empty, we print all functions, otherwise only @name
    //
    if (func_args_sorted_.empty()) throw std::runtime_error("no available functions");

    std::string type_str;
    ufunc *func_ptr;
    std::unordered_multiset<std::string> func_to_show;

    if (name.empty()) {
        py::print("Functions available in " + pkg_name_ + " package:");
    } else {
        load_string_to_container<std::unordered_multiset<std::string>>(func_to_show, name);
        for (const auto &el : func_to_show) {
            if (!func_args_sorted_.count(el)) throw std::invalid_argument(el + ": no such function in " + pkg_name_ + " package");
        }
    }

    // we iterate over functions in package
    for (const auto &[key, val] : func_args_sorted_) {
        if (!name.empty() && func_to_show.count(key) == 0) continue;
        func_ptr = func_ptrs_.at(key);

        if constexpr (use_lgretl == true) {
            print_function_signature(func_ptr, printer_.prn_ptr());
            printer_.print();
        } else {
            auto type = static_cast<GretlType>(user_func_get_return_type(func_ptr));
            py::print(key, "->", gretl_type_get_name(type));
            for (const auto &it : val) {
                type = static_cast<GretlType>(fn_param_type(func_ptr, it.second));
                auto default_value = fn_param_default(func_ptr, it.second);
                std::string default_value_str;

                if (default_value != NADBL && !(std::isnan)(default_value)) {
                    const char *tname = gretl_type_get_name(type);

                    if (!std::strcmp(tname, "int")) {
                        default_value_str = std::to_string(int(default_value));
                    } else if (!std::strcmp(tname, "bool")) {
                        default_value_str = default_value ? "true" : "false";
                    }
                }
                type_str = "(";
                py::print("  ", it.first, type_str.append(gretl_type_get_name(type)).append(")"),
                        (!default_value_str.empty() ? "default: " + default_value_str : "")
                        + std::string((fn_param_optional(func_ptr, it.second) ? "[optional]" : "")));
            }
            py::print();
        }
    }
}
template void GretlPackage::show_functions<true>(const std::string&);
template void GretlPackage::show_functions<false>(const std::string&);

py::list GretlPackage::get_functions (void) const
{
    if (func_args_.empty()) throw std::runtime_error("no available functions");

    py::list res;

    for (auto &it : func_args_) res.append(it.first);

    return res;
}

double GretlPackage::get_version (void) const
{
    return version_;
}

std::string GretlPackage::get_long_version (void) const
{
    return long_version_;
}

GretlPackage::FunctionArgsVec GretlPackage::get_func_args (const std::string &fun_name) const
{
    auto it = func_args_sorted_.find(fun_name);

    if (it != func_args_sorted_.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("there is no such function: " + fun_name);
    }
}

ufunc *GretlPackage::func_get_ptr (const std::string &name) const
{
    auto it = func_ptrs_.find(name);

    if (it != func_ptrs_.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("unknown function: " + name);
    }
}

GretlType GretlPackage::func_get_res_type (const std::string &name) const
{
    auto it = func_ptrs_.find(name);

    if (it != func_ptrs_.end()) {
        return static_cast<GretlType>(user_func_get_return_type(it->second));
    } else {
        throw std::invalid_argument("unknown function: " + name);
    }
}

GretlPackageRes GretlPackage::function_exec (const std::string &func_name, GretlDatasetPtr &dataset,
                                             GretlPrint &printer, const py::args &args, const py::kwargs &kwargs)
{
    // we prepare function to call
    fncall *func_call = user_func_get_fncall(func_get_ptr(func_name));

    if (!func_call) throw std::runtime_error("unable to load '" + func_name + "' function");

    py::kwargs merged_kwargs = kwargs;  // we copy kwargs to handle posssible args
    auto n_args = args.size();          // we handle args (positional parameters, if exist)

    if (n_args > 0) {
        auto func_args = func_args_sorted_.at(func_name);

        if (n_args > func_args.size())
            throw py::type_error("number of positional arguments bigger than total number of '" + func_name
                                 + "' function arguments (" + std::to_string(func_args.size()) + ")");
        for (size_t i = 0; i < n_args; ++i) {
            const std::string &arg_name = func_args.at(i).first;

            if (merged_kwargs.contains(arg_name)) throw py::type_error("multiple values for argument '" + arg_name + "'");
            merged_kwargs[arg_name.c_str()] = args[i];
        }
    }

    // we unpack parameters
    unpack_kwargs_package_(func_name, func_call, dataset, merged_kwargs);

    // we check dataset in the context of package's "data-requirement"
    if (!dataset && !(get_properties_<DataReq>("data-requirement") == FN_NODATA_OK)) throw std::runtime_error("No dataset is available");

    // we must set returning type
    GretlPackageRes res;
    gretl_list_ptr gr_list{nullptr};

    res.set_type(func_get_res_type(func_name));

    // initialize the plot queue before libgretl invokes gp-executor
    GretlPlot::init_queue();

    // function execution
    err_ = gretl_function_exec(func_call, res.type, dataset ? dataset->dset : nullptr, res.out_ptr(), printer.prn_ptr());
    if (err_) handle_gretl_error(err_);

    // execute plots generated by the function package
    GretlPlot::drain_queue();

    // handling special cases
    if (res.type == GRETL_TYPE_NUMERIC) {
        res.type = fncall_get_return_type(func_call); // for 'GRETL_TYPE_NUMERIC' as return type we must determine true type of returned object
    } else if (res.type == GRETL_TYPE_SERIES) {
        res.n = dataset->dset->n; // for GRETL_TYPE_SERIES we must record length
    }

    // transfer_ownership() must come after the GRETL_TYPE_NUMERIC resolution
    // above, since it captures res.type (used to pick the right free function)
    // by value - calling it earlier would free the returned object as the
    // wrong type. Without this, raw_ptr was never freed at all: nothing else
    // takes ownership of it (convert_GretlType_to_GretlTypes() only reads it
    // via copies), so every call to a contributed package's function leaked
    // its return value.
    res.transfer_ownership();

    return res;
}

std::filesystem::path GretlPackage::get_examples_path (const std::string &fname) const
{
    return std::filesystem::path(base_dir_) / "examples" / fname;
}

std::string GretlPackage::get_function_with_role (UfunRole role)
{
    return get_properties_<std::string>(package_role_get_key(role));
}

void GretlPackage::run_sample (void)
{
    //
    // we run sample script via gretl_execute_script()
    //
    std::filesystem::path sample_path = get_examples_path(pkg_name_ + "_sample.inp");

    if (!std::filesystem::exists(sample_path) || !std::filesystem::is_regular_file(sample_path)) handle_gretl_error(E_FOPEN, sample_path.string());
    err_ = gretl_execute_script<std::filesystem::path>(sample_path);
    if (err_) handle_gretl_error(err_);
}
