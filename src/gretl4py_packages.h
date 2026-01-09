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

namespace py = pybind11;

//
// Class to handle contributed packages
//
class GretlPackage {
private:
    using FunctionMap = std::map<std::string, ufunc*>;          // map of pointers to functions in package
    using FunctionArgs =
        std::map<std::string, std::map<std::string, int>>;      // map of arguments and their id-s for public functions
    using MainFuncNoOptDef = std::map<std::string, GretlType>;  // map of non-optional main_func arguments without default values

    static inline std::map<std::string, GretlPackagePtr> GPKGs_;// registry of loaded contributed packages

    const std::string pkg_name_;        // package name (base of filename)
    const std::string suffix_;          // filename suffix
    fnpkg *pkg_ = NULL;                 // pointer to package
    GretlPrint GP_ = GretlPrint();      // printer (instance wide)
    GretlBundle GB_;                    // info bundle [NOT USED YET]
    std::string main_func_ = "";        // name of main package function
    std::vector<std::string> jel_;      // vector of JEL codes
    bool defines_estimator_ = false;    // true if package implements estimation
    FunctionMap func_ptrs_;
    FunctionArgs func_args_;
    MainFuncNoOptDef main_func_nd_;
    std::string full_path_;             // path to package file (full)
    std::string base_dir_;              // path to the package
    int err_ = 0;

    void check_if_defines_estimator_ (void);

    template <typename T>
    T get_properties_ (const char *property);

    void unpack_kwargs_package_ (const std::string &func_name,
                                 fncall *func_call, GretlDatasetPtr &GD,
                                 const py::kwargs &kwargs);
public:
    GretlPackage (const std::string &pkg_name, const std::string &suffix);
    ~GretlPackage (void);

    // static methods
    static void clear_registry (void);

    static GretlPackagePtr load_package (const std::string &pkgname,
                                         const std::string &suffix = "");

    static bool package_loaded (const std::string &pkgname);

    int get_err (void) const;

    std::string repr_impl (void);

    void set_main (const std::string &name);

    std::string get_name (void) const;

    void show_functions (void) const;

    py::list get_functions (void) const;

    bool is_estimator (void) const;

    std::map<std::string, int> get_func_args (const std::string &fun_name) const;

    std::string main_func_get_name (void) const;

    ufunc *func_get_ptr (const std::string &name) const;

    GretlType func_get_res_type (const std::string &name) const;

    GretlPackageRes function_exec (const std::string &func_name,
                                   GretlDatasetPtr &GD, GretlPrint &GP,
                                   const py::kwargs &kwargs);

    std::string get_examples_path (const std::string &fname);

    std::string get_function_with_role (UfunRole role);
};

#endif // GRETL4PY_PACKAGES_H
