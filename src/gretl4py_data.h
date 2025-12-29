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
#include "gretl_calls.h"
extern "C" {
    #include <gretl/gretl_func.h>
}

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
#define is_pysequence(o) (py::isinstance<py::sequence>(o))
#define is_pyGretlBundle(o) (py::isinstance<GretlBundle>(o))
#define is_NumPyArrayFC(m) ((m.flags() & py::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_))

#define SET_OPT(opt, flag) opt = static_cast<gretlopt>((opt) | (flag))

//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////
void NumPyArray_extract (const py::array &arr, NumPyArrayView &view);

template <typename T>
void NumPyArray_to_gretl_matrix (NumPyArrayView &view, T *dst,
                                 int offset = 0, bool onecol = false);

GretlTypesPtr convert_GretlType_to_GretlTypes (GretlType t, void *gr_obj_ptr,
                                               const int n, int &err);

template <typename PyWrapped, typename Item, typename ContainerType>
void move_to_python_container (PyWrapped &wrapped_obj, Item &&item,
                               const char *container, const std::string &key = "");

py::object variant_to_pyobject (GretlTypes &&val);

PythonArgInfo inspect_pyobject (py::handle obj);

std::string gretl4py_trim (std::string &s);

template <typename T>
void get_object_from_dict (const py::dict &dict, const char *key, T &target);

//
// Class to handle gretl 'DATASET' type
//
class GretlDataset : public std::enable_shared_from_this<GretlDataset>
{
private:
    static inline int next_obj_id_ = 1;
    const int id_;                          // dataset's id
    std::unordered_set<int> linked_models_; // models (id-s) which use this dataset
    GretlPrint GP_ = GretlPrint();          // printer (instance wide)
    mutable GretlTypesPtr RES_other_;       // other results (accessor, tests, ets)
    gretlopt opt_ = OPT_NONE;               // instance-wide options
    mutable int err_ = 0;

    void allocate_dset_ (void);

    void free_dset_ (void);

    void add_or_replace_allocated_series_ (gretl_double_ptr data,
                                           const std::string &name, int n,
                                           gretl_gchar_ptr label = nullptr);
public:
    bool is_default = false;
    DATASET *dset = NULL;
    std::string source;

    GretlDataset (const std::string &fname,
                  const py::dict &kwargs);      // we load dataset from file (with optional flags as kwargs)
    GretlDataset (const py::dict &pydict);      // we load dataset from Python's dictionary
    GretlDataset (const GretlDataset &src);     // copy constructor (always deep copy)
    virtual ~GretlDataset (void);               // virtual, because GretlModel use it as std::shared_ptr

    // instance management
    int get_id (void) const;

    std::string repr (void) const;

    std::unordered_set<int> linked_models_list (void) const;

    void linked_models_add (const int model_id);

    void linked_models_del (const int model_id);

    void print (const py::kwargs &kwargs);

    void set_as_default (void);

    // exporters
    py::object get_accessor (const std::string &name);

    py::dict to_dict (void) const;

    void to_file (const std::string &fname, const std::string &vars,
                  const py::kwargs &kwargs);

    // dataset information
    bool is_panel (void) const;

    py::list varnames (void) const;

    int get_series_id (const std::string &series, const bool verbose = true) const;

    // dataset management
    void setobs (const std::string &periodicity, const std::string &startobs,
                 const py::kwargs &kwargs);
    //void sample (const py::kwargs &kwargs);

    void smpl (const std::string &startobs, const std::string &endobs,
               const py::kwargs &kwargs);

    template <typename T>
    GretlDatasetPtr getitem_impl (const T &slice);

    void gretl_series_set_label (const int number, char *label);

    char *add_series (gretl_double_ptr data,
                      const std::string &name, int n,
                      gretl_gchar_ptr label = nullptr);

    void add_series (const py::array &data, const std::string &name);

    void new_series (const std::string &formula);

    void new_series (const std::string &name, const std::string &value);

    void new_list (const std::string &formula);

    void new_list (const std::string &name, const std::string &value);

    py::object get_series (const std::string &series) const;

    // tests
    void test (const std::string &type, const std::string &vars,
               const py::kwargs &kwargs);

    // transformations
    template <GretlCmdIndex CI>
    void transforms (const std::string &varlist, const py::kwargs &kwargs);

    template <GretlCmdIndex CI>
    void transforms_no_opts (const std::string &varlist);

    void lags (const std::string &varlist, int order);

    // filters, correlations, etc
    DNumPyArrayFC hpfilt (const std::string &y, double hp_lambda = NADBL,
                          bool trend = false, bool one_sided = false);

    DNumPyArrayFC bwfilt (const std::string &y, int n, double omega);

    DNumPyArrayFC bkfilt (const std::string &y, int f1 = 0,
                          int f2 = 0, int k = 0);

    py::object corr (const std::string &varlist, const py::kwargs &kwargs);

    py::object cov (const std::string &vars);

    py::object hurst (const std::string &series);

    py::object xcorrgm (const std::string &vars, int order,
                        const py::kwargs &kwargs);

    py::object pca (const std::string &varlist, const py::kwargs &kwargs);
};

//
// Class to handle 'gretl_bundle' type
//
class GretlBundle
{
private:
    bool free_b_ = true;                            // if *b should be freed
    gretl_bundle *b_ = NULL;                        // libgretl bundle pointer
    py::dict as_dict_;                              // Python's dict() reprezentation
    int err_ = 0;

    template <typename T>
    void extract_ (const T &t);
public:
    GretlBundle (void);                             // default constructor with empty bundle
    GretlBundle (gretl_bundle *src);                // incoming already existing bundle Note: src must be deallocated by libgretl
    GretlBundle (MODEL *model, DATASET *dset);      // we produce bundle from MODEL
    GretlBundle (GRETL_VAR *model, DATASET *dset);  // we produce bundle from VAR/VECM
    GretlBundle (equation_system *model);           // we produce bundle from SYSTEM
    ~GretlBundle (void);

    int get_err (void) const;

    py::dict to_dict (void);                        // bundle-to-dict mapper

    void python_register (py::object &obj);         // register bundle contents in python object

    std::string repr_impl (void);                   // bundle printing via __repr__

    gretl_bundle *get_ptr (void) const;             // raw gretl_bundle pointer

    py::list keys (void);

    template <typename T>
    T print (bool tree = false);                    // bundle printer

    template <typename T>
    T get_element (const std::string &key);         // get bundle element by name

    template <typename T>
    void set_element (const std::string &key, const T &t);
};

//
// This is general purpose py::object/py::handle to C++ types mapper
//
class MapPyToCpp
{
private:
    std::string as_str_;

    template <typename T>
    void convert_ (const T *pyobj);
public:
    int err = 0;

    MapPyToCpp (const std::optional<py::object> &pyobject);
    MapPyToCpp (const py::handle &pyhandle);

    inline std::string to_string (void) const { return as_str_; }

    inline std::string_view to_string_view (void) const { return static_cast<std::string_view>(as_str_); }
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

#endif // GRETL4PY_DATA_H
