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

#ifndef GRETL4PY_PACKAGES_H
#define GRETL4PY_PACKAGES_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "gretl4py_data.h"
#include "gretl_calls.h"
extern "C" {
    #include <gretl/gretl_func.h>
}
#include <filesystem>

namespace py = pybind11;

//
// Class to handle contributed packages
//
class GretlPackage {
private:
    using FunctionMap = std::map<std::string, ufunc*>;                      // map of pointers to functions in package
    using FunctionArgs = std::map<std::string, std::map<std::string, int>>; // map of arguments and their id-s for public functions
    using MainFuncNoOptDef = std::map<std::string, GretlType>;              // map of non-optional main_func arguments without default values
    using FunctionArgsVec = std::vector<std::pair<std::string, int>>;       // vector of arguments and their id-s for public functions
    using FunctionArgsSort = std::map<std::string, FunctionArgsVec>;        // map of sorted arguments and their id-s for public functions

    static inline std::map<std::string, GretlPackagePtr> gpkgs_;// registry of loaded contributed packages

    const std::string pkg_name_;        // package name (base of filename)
    const std::string suffix_;          // filename suffix
    fnpkg *pkg_ = nullptr;              // pointer to package
    GretlPrint printer_ = GretlPrint(); // printer (instance wide)
    GretlBundle gb_;                    // info bundle
    FunctionMap func_ptrs_;             // pointers to package functions
    FunctionArgs func_args_;            // arguments of package functions (non sorted)
    FunctionArgsSort func_args_sorted_; // arguments of package functions (sorted)
    std::string full_path_;             // path to package file (full)
    std::string base_dir_;              // path to the package
    gretl_char_ptr pdfname_ = nullptr;  // name of pdf doc (if exists)
    double version_;                    // package version (numeric)
    std::string long_version_;          // package "long" version (string)
    int err_ = 0;

    GretlPackage (const std::string &pkg_name, const std::string &suffix, bool print_summary = true);

    template <typename T>
    T get_properties_ (const char *property);

    void unpack_kwargs_package_ (const std::string &func_name, fncall *func_call, GretlDatasetPtr &dataset, const py::kwargs &kwargs);
public:
    ~GretlPackage (void);

    // static methods
    static void clear_registry (void);

    static GretlPackagePtr load_package (const std::string &pkgname, const std::string &suffix = "", bool print_summary = true);

    static bool package_loaded (const std::string &pkgname);

    // instance methods
    int get_err (void) const;

    std::string repr_impl (void);

    py::list get_help (void);

    std::string get_name (void) const;

    template <bool use_lgretl>
    void show_functions (const std::string &name = "");

    py::list get_functions (void) const;

    bool is_estimator (void) const;

    double get_version (void) const;

    std::string get_long_version (void) const;

    FunctionArgsVec get_func_args (const std::string &fun_name) const;

    std::string main_func_get_name (void) const;

    ufunc *func_get_ptr (const std::string &name) const;

    GretlType func_get_res_type (const std::string &name) const;

    GretlPackageRes function_exec (const std::string &func_name, GretlDatasetPtr &dataset, GretlPrint &printer, const py::args &args, const py::kwargs &kwargs);

    std::filesystem::path get_examples_path (const std::string &fname) const;

    std::string get_function_with_role (UfunRole role);

    void run_sample (void);
};

#endif // GRETL4PY_PACKAGES_H
