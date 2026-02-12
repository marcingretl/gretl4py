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

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "gretl4py_types.h"
#include "gretl4py_packages.h"
#include "gretl4py_utils.h"
extern "C" {
    #include <gretl/uservar.h>
    #include <gretl/gretl_func.h>
    #include <gretl/gretl_typemap.h>
}
#include <iomanip>
#include <filesystem>

extern std::unordered_set<std::string> user_vars;

//
// GretlPackage class methods
//
GretlPackage::GretlPackage (const std::string &pkg_name, const std::string &suffix,
                            bool print_summary) : pkg_name_(pkg_name), suffix_(suffix)
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
        UFUN_BUNDLE_PLOT, UFUN_GUI_MAIN, UFUN_GUI_PRECHECK,
        UFUN_PLOT_PRECHECK, UFUN_UI_MAKER
    };
    std::unordered_set<std::string> excluded_functions;
    std::ostringstream oss;
    char path[MAXLEN];
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
    if (pkg_name != function_package_get_name(pkg_)) throw std::runtime_error("package name mismatch: internal: '" + std::string(function_package_get_name(pkg_)) + "', provided: '" + pkg_name_ + "'");
    err_ = bundle_function_package_info(full_path_.c_str(), GB_.get_ptr());
    if (err_) handle_gretl_error(err_);
    version_ = function_package_get_version(pkg_);
    long_version_ = get_properties_<std::string>("version");

    // we initialize package functions
    function_names_init();

    // we look up for JEL codes
    jel_ = get_properties_<std::vector<std::string>>("tags");

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

    // we try to register package 'main' function (i.e. the one to call via ::fit())
    if (func_ptrs_.size() == 1) {
        set_main(func_ptrs_.begin()->first);
    } else {
        // some heuristics here
        // at first: name of the package
        auto it = func_ptrs_.find(pkg_name);
        
        if (it != func_ptrs_.end()) {
            set_main(it->first);
        } else {
            // second: we look for 'estimate' in function name
            for (auto it : func_ptrs_) {
                if (it.first.find("estimate") != std::string::npos) set_main(it.first);
            }
        }
    }
    check_if_defines_estimator_();

    // we grab name of PDF info
    if (function_package_has_PDF_doc(pkg_, &raw_ptr)) pdfname_.reset(raw_ptr);

    // package summary
    if (print_summary) {
        oss << std::fixed << std::setprecision(2) << version_;
        py::print("We use '" + pkg_name_ + "' package, version: " + oss.str()
                  + ", JEL codes: " + get_properties_<std::string>("tags")
                  + " (" + get_properties_<std::string>("author") + ")"
                  + "\n  main function: " + (main_func_.empty() ? "none" : main_func_)
                  + ", provides estimator: " + (defines_estimator_ ? "yes" : "no"));
        if (main_func_.empty()) py::print("warning: package main function was not found: please use ::set_main()");
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
    GPKGs_.clear();
}

GretlPackagePtr GretlPackage::load_package (const std::string &pkgname,
                                            const std::string &suffix,
                                            bool print_summary)
{
    //
    // here we register contributed package
    // at first, we look up for package in registry
    //
    // at first we check if package already exists
    auto it = GPKGs_.find(pkgname);
    
    if (it != GPKGs_.end()) return it->second;

    // now we load the package
    GretlPackagePtr pkg(new GretlPackage(pkgname, suffix, print_summary));
    GPKGs_.emplace(pkgname, pkg);

    return pkg;
}

bool GretlPackage::package_loaded (const std::string &pkgname)
{
    //
    // here we check if give package exists in registry
    //
    return GPKGs_.count(pkgname) == 0 ? false : true;
}

void GretlPackage::check_if_defines_estimator_ (void)
{
    static constexpr std::array<const char*, 24> estimation_codes = {
        "C13", "C14", "C15", "C20", "C21", "C22", "C23", "C24",
        "C25", "C26", "C30", "C31", "C32", "C33", "C34", "C35",
        "C36", "C40", "C41", "C51", "C52", "C53", "C54", "C58"
    };

    for (const auto &it : jel_) {
        if (std::find(estimation_codes.begin(), estimation_codes.end(), it) != estimation_codes.end()) {
            defines_estimator_ = true;
            return;
        }
    }
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
        err_ = function_package_get_properties(pkg_, property, &res, NULL);
    } else {
        err_ = function_package_get_properties(pkg_, property, &val_raw, NULL);
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

void GretlPackage::unpack_kwargs_package_ (const std::string &func_name,
                                           fncall *func_call, GretlDatasetPtr &GD,
                                           const py::kwargs &kwargs)
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
            GD = val.cast<GretlDatasetPtr>();
            continue;
        } else if (key_str == "quiet" || key_str == "silent") {
            // do something elsewhere
            continue;
        } else {
            if (!std::any_of(func_name_args.begin(),
                             func_name_args.end(),
                             [&](const auto &p){ return p.first == key_str; }))
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
            // if parameter is [optional] (i.e. does not have default value), we must set it to GRETL_TYPE_NONE
            if (fn_param_optional(func_get_ptr(func_name), arg_no)) {
                err_ = push_function_arg(func_call, key.c_str(),
                                         NULL, GRETL_TYPE_NONE, NULL);
                if (err_) handle_gretl_error(err_);
            }
            continue;
        }
        auto val = kwargs[py::cast(key)];
        PythonArgInfo arginfo = inspect_pyobject(val);
        arg_type = GRETL_TYPE_NONE;
        arg_value = NULL;
        auto key_type = static_cast<GretlType>(fn_param_type(func_get_ptr(func_name), arg_no));

        if (key_type == GRETL_TYPE_SERIES && GD) {
            // special case: for series we swap situation and
            // evaluate arg via type declared in a function
            arg_name = py::cast<std::string>(val);
            int vnum = GD->get_series_id(arg_name);
            err_ = push_function_arg(func_call, arg_name.c_str(),
                                     NULL, GRETL_TYPE_USERIES, &vnum);
            if (err_) handle_gretl_error(err_);
            continue;
        } else if (arginfo.gtype == GRETL_TYPE_BUNDLE) {
            // note: here we deal with both: GRETL_TYPE_BUNDLE and GRETL_TYPE_BUNDLE_REF
            // final distinction is made by key_type
            auto gb = val.cast<GretlBundlePtr>();
            err_ = push_function_arg(func_call, NULL, NULL,
                                     key_type, gb->get_ptr());
            if (err_) handle_gretl_error(err_);
            continue;
        } else if (arginfo.gtype == GRETL_TYPE_MATRIX) {
            NumPyArrayView arr_view;
            gretl_matrix_ptr mat{nullptr};
            auto arr = val.cast<py::array>();

            NumPyArray_extract(arr, arr_view);
            if (!arr_view.data) throw std::runtime_error("NumPy array: null pointer.");
            mat.reset(gretl_matrix_alloc(arr_view.rows, arr_view.cols));
            NumPyArray_to_gretl_matrix<gretl_matrix>(arr_view, mat.get());
            err_ = push_function_arg(func_call, key.c_str(), NULL,
                                     GRETL_TYPE_MATRIX, mat.release());
            if (err_) handle_gretl_error(err_);
            continue;
        } else if (arginfo.gtype == GRETL_TYPE_STRING) {
            // special case: we need to know if we deal with
            // user defined variable uv or string per se
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
                arg_type = is_ref_user_var ?
                    gretl_type_get_ref_type(user_var_get_type(uv))
                    : user_var_get_type(uv);
                err_ = push_function_arg(func_call, arg_name.c_str(), uv,
                                         arg_type, user_var_get_value(uv));
                if (err_) handle_gretl_error(err_);
                continue;
            } else {
                char *arg_name_copy = strdup(arg_name.c_str());
                arg_value = static_cast<void*>(arg_name_copy);
                arg_type = arginfo.gtype;
            }
        } else if (arginfo.gtype == GRETL_TYPE_BOOL ||
                   arginfo.gtype == GRETL_TYPE_DOUBLE ||
                   arginfo.gtype == GRETL_TYPE_INT) {
            darg_value = py::cast<double>(val);
            arg_value = &darg_value;
            arg_type = GRETL_TYPE_DOUBLE;
        } else {
            throw std::runtime_error("variable " + key +
                                     ": unsupported type: "
                                     + arginfo.pytype);
        }
        err_ = push_function_arg(func_call, key.c_str(),
                                 NULL, arg_type, arg_value);
        if (err_) handle_gretl_error(err_);
    }
}

int GretlPackage::get_err (void) const
{
    return err_;
}

std::string GretlPackage::repr_impl (void)
{
    err_ = print_function_package_info(full_path_.c_str(), 0, GP_.prn_ptr());
    if (err_) handle_gretl_error(err_);

    return GP_.get_buffer();
}

py::list GretlPackage::get_help (void)
{
    //
    // return either text help or path to pdf
    //
    py::list RES;

    if (pdfname_) {
        std::filesystem::path pdf_path = std::filesystem::path(base_dir_) / pdfname_.get();

        RES.append("pdf");
        RES.append(pdf_path.string());
    } else {
        RES.append("txt");
        err_ = print_function_package_help(full_path_.c_str(),
                                           NULL, GP_.prn_ptr());
        if (err_) handle_gretl_error(err_);
        RES.append(GP_.get_buffer());
    }

    return RES;
}

void GretlPackage::set_main (const std::string &name)
{
    // we register main_func
    auto it = func_ptrs_.find(name);
    if (it != func_ptrs_.end()) {
        main_func_ = name;
    } else {
        throw std::invalid_argument("there is no such function: " + name);
    }

    // we register all non-optional arguments with types
    if (!main_func_nd_.empty()) main_func_nd_.clear();
    auto main_func_args = func_args_.at(name);
    for (const auto &[key, val] : main_func_args) {
        if (!fn_param_optional(it->second, val) && !fn_param_has_default(it->second, val)) {
            main_func_nd_.emplace(key, static_cast<GretlType>(fn_param_type(it->second, val)));
        }
    }
}

std::string GretlPackage::get_name (void) const
{
    return pkg_name_;
}

void GretlPackage::show_functions (const std::string &name)
{
    //
    // prints the signatures of all public functions provided by the package
    // in @name is empty, we print all functions, otherwise only @name
    //
    if (func_args_sorted_.empty()) throw std::runtime_error("no available functions");

    GretlType type;
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
        type = static_cast<GretlType>(user_func_get_return_type(func_ptr));
        py::print(key, "->", gretl_type_get_name(type), (key == main_func_ ? "[main] " : ""));
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
            py::print("  ", it.first,
                      type_str.append(gretl_type_get_name(type)).append(")"),
                      (!default_value_str.empty() ? "default: " + default_value_str : "")
                      + std::string((fn_param_optional(func_ptr, it.second) ? "[optional]" : "")));
        }
    }
    py::print();
}

py::list GretlPackage::get_functions (void) const
{
    if (func_args_.empty()) throw std::runtime_error("no available functions");

    py::list res;

    for (auto &it : func_args_) res.append(it.first);

    return res;
}

bool GretlPackage::is_estimator (void) const
{
    return defines_estimator_;
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

std::string GretlPackage::main_func_get_name (void) const
{
    return main_func_;
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

GretlPackageRes GretlPackage::function_exec (const std::string &func_name,
                                             GretlDatasetPtr &GD, GretlPrint &GP,
                                             const py::args &args,
                                             const py::kwargs &kwargs)
{
    // we prepare function to call
    fncall *func_call = user_func_get_fncall(func_get_ptr(func_name));
    if (!func_call) throw std::runtime_error("unable to load '" + func_name + "' function");

    // we copy kwargs to handle posssible args
    py::kwargs merged_kwargs = kwargs;

    // we handle args (positional parameters, if exist)
    auto n_args = args.size();

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
    unpack_kwargs_package_(func_name, func_call, GD, merged_kwargs);

    // we check dataset in the context of package's "data-requirement"
    if (!GD && !(get_properties_<DataReq>("data-requirement") == FN_NODATA_OK)) throw std::runtime_error("No dataset is available");

    // we must set returning type (must be released at the end)
    GretlPackageRes res;
    gretl_list_ptr gr_list{nullptr};

    res.set_type(func_get_res_type(func_name));

    // function execution
    err_ = gretl_function_exec(func_call, res.type, GD ? GD->dset : NULL,
                               res.out_ptr(), GP.prn_ptr());
    if (err_) handle_gretl_error(err_);

    // for GRETL_TYPE_SERIES we must record length
    if (res.type == GRETL_TYPE_SERIES) res.n = GD->dset->n;

    return res;
}

std::string GretlPackage::get_examples_path (const std::string &fname)
{
    std::string examples_path = base_dir_;

#if defined(_WIN32)
    examples_path.append("\\examples");
#else
    examples_path.append("/examples");
#endif // defined

    if (!fname.empty()) {
#if defined(_WIN32)
    examples_path.append("\\").append(fname);
#else
    examples_path.append("/").append(fname);
#endif // defined
    }

    return examples_path;
}

std::string GretlPackage::get_function_with_role (UfunRole role)
{
    return get_properties_<std::string>(package_role_get_key(role));
}
