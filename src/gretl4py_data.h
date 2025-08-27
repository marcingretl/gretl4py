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

#ifndef GRETL4PY_DATA_H
#define GRETL4PY_DATA_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "gretl4py_types.h"
#include <gretl/libgretl.h>
#include <gretl/matrix_extra.h>
#include <gretl/genmain.h>
#include <gretl/objstack.h>
extern "C" {
    #include <gretl/gretl_func.h>
}
#include <iostream>
#include <unordered_set>
#include <filesystem>

namespace py = pybind11;

//////////////////////////////////////////////////
// macros
//////////////////////////////////////////////////
#define is_pybool(o) (py::isinstance<py::bool_>(o))
#define is_pyint(o) (py::isinstance<py::int_>(o))
#define is_pyfloat(o) (py::isinstance<py::float_>(o))
#define is_pystr(o) (py::isinstance<py::str>(o))
#define is_pydict(o) (py::isinstance<py::dict>(o))
#define is_pytuple(o) (py::isinstance<py::tuple>(o))
#define is_pylist(o) (py::isinstance<py::list>(o))
#define is_pynumarray(o) (py::isinstance<py::array>(o))
#define is_pynumarrayd(o) (py::isinstance<py::array_t<double>>(o))
#define is_pynumeric(o) (is_pyint(o) || is_pyfloat(o))
#define is_pyGretlBundle(o) (py::isinstance<GretlBundle>(o))

#define SET_OPT(opt, flag) opt = static_cast<gretlopt>((opt) | (flag))

//////////////////////////////////////////////////
// functions
// Note: some signature(s) may be at the bottom of this header
//////////////////////////////////////////////////
void handle_gretl_error (int err);

void NumPyArray_extract (const py::array &arr, NumPyArrayView &view);

template<typename T>
void NumPyArray_to_gretl_matrix (NumPyArrayView &view, T *dst,
                                 int offset = 0, bool onecol = false);

GretlTypesPtr gretl_generate_gretl_object (const char *s, int &err,
                                           GretlObjType obj_type = GRETL_OBJ_INFO,
                                           DATASET *dset = NULL, void *model = NULL);

void gretl_valid_long_opt (gretlopt &opt, GretlCmdIndex ci,
                           const std::string &key, const py::handle &val,
                           const char *ext_val = nullptr);

void unpack_kwargs_test (gretlopt &opt, const std::string &type,
                         const py::kwargs &kwargs, int &order);

gretl_list_ptr list_from_formula (const std::string &formula,
                                  DATASET *dset, int &err);

GretlTypesPtr convert_GretlType_to_GretlTypes (GretlType t, void *gr_obj_ptr,
                                               const int n, int &err);

template<typename PyWrapped, typename Item, typename ContainerType>
void move_to_python_container (PyWrapped &wrapped_obj, Item &&item,
                               const char *container, const std::string &key = "");

py::object variant_to_pyobject (GretlTypes &&val);

PythonArgInfo inspect_pyobject (py::handle obj);

//
// Class to handle printing
//
class GretlPrint
{
private:
    PRN *prn = NULL;
    PrnType prn_type;
public:
    int err = 0;

    GretlPrint (PrnType prn_type = GRETL_PRINT_BUFFER);  // by default we use buffered printer
    ~GretlPrint (void);

    PRN *prn_ptr (void);

    void print (void);

    const char *get_buffer (void);
};


//
// Class to handle gretl 'DATASET' type
//
class GretlDataset : public std::enable_shared_from_this<GretlDataset>
{
private:
    static inline int next_obj_id = 1;
    const int id;                           // dataset's id
    std::unordered_set<int> linked_models;  // models (id-s) which use this dataset
    GretlPrint GP = GretlPrint();           // printer (instance wide)
    mutable GretlTypesPtr RES_other;        // other results (accessor, tests, ets)

    gretlopt opt = OPT_NONE;

    void allocate_dset (void);

    void free_dset (void);

    void add_or_replace_allocated_series (gretl_double_ptr data,
                                          const std::string &name, int n,
                                          gretl_gchar_ptr label = nullptr);

    gretlopt setobs_opt_from_structure (int structure);
public:
    mutable int err = 0;
    bool is_default = false;
    DATASET *dset = NULL;
    std::string source;

    GretlDataset (const std::string &fname,
                  const py::dict &kwargs);      // we load dataset from file (with optional flags as kwargs)
    GretlDataset (const py::dict &pydict);      // we load dataset from Python's dictionary
    virtual ~GretlDataset (void);               // virtual, because GretlModel use it as std::shared_ptr

    py::dict to_dict (void) const;

    void to_file (const std::string &fname) const;

    std::string repr (void) const;

    void print (const py::kwargs &kwargs);

    void sample (const py::kwargs &kwargs);

    void set_as_default (void);

    void setobs (const py::kwargs &kwargs);

    char *add_series (gretl_double_ptr data,
                      const std::string &name, int n,
                      gretl_gchar_ptr label = nullptr);

    void add_series (const py::array &data, const std::string &name);

    void new_series (const std::string &formula);

    void new_series (const std::string &name, const std::string &value);

    void new_list (const std::string &formula);

    void new_list (const std::string &name, const std::string &value);

    void gretl_series_set_label (const int number, char *label);

    py::list varnames (void) const;

    int get_id (void) const;

    int get_series_id (const std::string &series, const bool verbose = true) const;

    void linked_models_add (const int model_id);

    void linked_models_del (const int model_id);

    std::unordered_set<int> linked_models_list (void) const;

    py::object get_accessor (const std::string &accessor) const;

    void test (const std::string &type, const std::string &vars,
               const py::kwargs &kwargs);

    py::object get_series (const std::string &series) const;

    DNumPyArrayFC hpfilt (const std::string &series, double hp_lambda = NADBL,
                          bool trend = false, bool one_sided = false);

    DNumPyArrayFC bwfilt (const std::string &series, int order, double omega);

    DNumPyArrayFC bkfilt (const std::string &series, int f1 = 0,
                          int f2 = 0, int k = 0);
};

//
// Class to handle 'gretl_bundle' type
//
class GretlBundle
{
private:
    bool free_b = true;
    gretl_bundle *b = NULL;
    py::dict as_dict;

    template<typename T>
    void extract (const T &t);
public:
    int err = 0;

    GretlBundle (void);                             // default constructor with empty bundle
    GretlBundle (gretl_bundle *src);                // incoming already existing bundle Note: src must be deallocated by libgretl
    GretlBundle (MODEL *model, DATASET *dset);      // we produce bundle from MODEL
    GretlBundle (GRETL_VAR *model, DATASET *dset);  // we produce bundle from VAR/VECM
    ~GretlBundle (void);

    py::dict to_dict (void);                        // bundle-to-dict mapper

    void python_register (py::object &obj);         // register bundle contents in python object

    void print (void);

    std::string repr (void);

    gretl_bundle *get_ptr (void) const;             // raw gretl_bundle pointer

    py::list keys (void);

    template<typename T>
    T get_element (const std::string &key);         // get bundle element by name

    template<typename T>
    void set_element (const std::string &key, const T &t);
};

//
// This is general purpose py::object/py::handle to C++ types mapper
//
class MapPyToCpp
{
private:
    std::string as_str;

    template<typename T>
    void convert (const T *pyobj);
public:
    int err = 0;

    MapPyToCpp (const std::optional<py::object> &pyobject);
    MapPyToCpp (const py::handle &pyhandle);

    inline std::string to_string (void) const { return as_str; }

    inline std::string_view to_string_view (void) const { return static_cast<std::string_view>(as_str); }
};

//
// Class to handle contributed packages
//
class GretlPackage {
private:
    using FunctionMap = std::map<std::string, ufunc*>;      // map of pointers to functions in package
    using FunctionArgs =
        std::map<std::string, std::map<std::string, int>>;  // map of arguments and their id-s for public functions
    using MainFuncNoOptDef = std::map<std::string, GretlType>;// map of non-optional main_func arguments without default values

    const std::string pkg_name;     // package name (base of filename)
    const std::string suffix;       // filename suffix
    fnpkg *pkg = NULL;              // pointer to package
    std::string main_func = "";     // name of main package function
    std::vector<std::string> jel;   // vector of JEL codes
    bool defines_estimator = false; // true if package implements estimation
    FunctionMap func_ptrs;
    FunctionArgs func_args;
    MainFuncNoOptDef main_func_nd;
    std::string base_dir;           // path to the package
    int err = 0;

    void check_if_defines_estimator (void);

    template<typename T>
    T get_properties (const char *property);

    void unpack_kwargs_package (const std::string &func_name,
                                fncall *func_call,
                                GretlDatasetPtr &GD,
                                const py::kwargs &kwargs);
public:
    // Note: constructor does everything: load pkg, scan functions, register self
    GretlPackage (const std::string &pkg_name, const std::string &suffix);
    ~GretlPackage (void);

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

// implemented in gretl_calls.cpp
extern GretlDatasetPtr DSET;

void set_default_dset(GretlDatasetPtr GD);

GretlPackagePtr load_package (const std::string &pkgname,
                              const std::string &suffix = "");

template<GretlType GR_TYPE>
void gretl_generate (const std::string &formula, GretlDatasetPtr GD = DSET,
                     const std::string &name = {});

#endif // GRETL4PY_DATA_H
