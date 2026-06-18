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
#include <gretl/gretl_matrix.h>
extern "C" {
    #include <gretl/gretl_func.h>
}

namespace py = pybind11;

//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////
template <typename T>
void NumPyArray_to_gretl_storage (NumPyArrayView &view, T *dst, int offset = 0, bool onecol = false);

//////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////
//
// Class to handle gretl 'DATASET' type
//
class GretlDataset : public std::enable_shared_from_this<GretlDataset>
{
private:
    static inline int next_obj_id_ = 1;
    static inline std::map<int, std::weak_ptr<GretlDataset>> dataset_;  // datasets registry
    static inline std::weak_ptr<GretlDataset> default_dataset_;         // default dataset
    const int id_;                                                      // dataset's id
    const bool from_file_ = false;                                      // if dataset was loaded from file
    std::unordered_set<int> linked_models_;                             // models (id-s) which use this dataset
    mutable GretlPrint printer_ = GretlPrint();                         // printer (instance wide)
    mutable GretlTypesPtr res_other_;                                   // other results (accessor, tests, etc)
    gretlopt opt_ = OPT_NONE;                                           // instance-wide options
    std::vector<std::string> obs_labels_;                               // observation labels
    mutable int err_ = 0;

    GretlDataset (const std::string &fname, const py::dict &kwargs);    // load dataset from file (with optional flags as kwargs)
    GretlDataset (const py::dict &pydict);                              // load dataset from Python's dictionary
    GretlDataset (const GretlDataset &src);                             // copy constructor (always deep copy)

    static GretlDatasetPtr get_dset_by_id_ (int id);

    static void register_dset_ (const GretlDatasetPtr dataset);

    void allocate_dset_ (void);

    void free_dset_ (void);

    void add_or_replace_allocated_series_ (gretl_double_ptr data, const std::string &name, int n,
                                           gretl_gchar_ptr label = nullptr);

    void set_obs_labels_ (void);
public:
    bool is_default = false;
    DATASET *dset = nullptr;
    std::string source;

    virtual ~GretlDataset (void);           // virtual, because GretlModel use it as std::shared_ptr

    // static methods
    static GretlDatasetPtr clone (const GretlDataset &src);

    static GretlDatasetPtr get_data (const py::object &src, const std::string &source, const py::kwargs &kwargs);

    static std::set<int> get_dset_ids (void);

    static GretlDatasetPtr default_dataset (void);

    // instance management
    int get_id (void) const noexcept;

    std::string repr (void) const;

    std::unordered_set<int> linked_models_list (void) const;

    void linked_models_add (const int model_id);

    void linked_models_del (const int model_id);

    void print (const std::string &varlist, const py::kwargs &kwargs);

    void set_as_default (void);

    // exporters
    py::object get_accessor (const std::string &name);

    py::dict to_dict (void) const;

    void to_file (const std::string &filename, const std::string &varlist, const py::kwargs &kwargs);

    // dataset information
    bool is_panel (void) const;

    py::list varnames (void) const;

    int get_series_id (const std::string &series, const bool verbose = true) const;

    py::list get_obs_labels (void) const;

    std::string summary (bool print = true) const;

    // dataset management
    void setobs (const std::string &periodicity, const std::string &startobs, const py::kwargs &kwargs);

    void smpl (const std::string &startobs, const std::string &endobs, const py::kwargs &kwargs);

    template <typename T>
    GretlDatasetPtr getitem_impl (const T &slice);

    void gretl_series_set_label (const int number, char *label);

    char *add_series (gretl_double_ptr data, const std::string &name, int n, gretl_gchar_ptr label = nullptr);

    void add_series (const py::array &data, const std::string &name);

    void new_series (const std::string &formula);

    void new_series (const std::string &name, const std::string &value);

    void new_list (const std::string &formula);

    void new_list (const std::string &name, const std::string &value);

    void rename (int series_id, const std::string &newname, const py::kwargs &kwargs);

    py::object get_series (const std::string &series) const;

    // tests
    void test (const std::string &type, const std::string &varlist, const py::kwargs &kwargs);

    // transformations
    template <GretlCmdIndex CI>
    void transforms (const std::string &varlist, const py::kwargs &kwargs);

    template <GretlCmdIndex CI>
    void transforms_no_opts (const std::string &varlist);

    void lags (const std::string &varlist, int order);

    // filters, correlations, etc
    DNumPyArrayFC hpfilt (const std::string &y, double hp_lambda = NADBL, bool trend = false, bool one_sided = false);

    DNumPyArrayFC bwfilt (const std::string &y, int n, double omega);

    DNumPyArrayFC bkfilt (const std::string &y, int f1 = 0, int f2 = 0, int k = 0);

    py::object corr (const std::string &varlist, const py::kwargs &kwargs);

    py::object cov (const std::string &vars);

    py::object hurst (const std::string &series);

    py::object xcorrgm (const std::string &vars, int order, const py::kwargs &kwargs);

    py::object pca (const std::string &varlist, const py::kwargs &kwargs);
};

//
// Class to handle 'gretl_bundle' type
//
class GretlBundle
{
private:
    bool free_b_ = true;                            // if *b should be freed
    gretl_bundle *b_ = nullptr;                     // libgretl bundle pointer
    py::dict as_dict_;                              // Python's dict() representation
    int err_ = 0;

    template <typename T>
    void extract_ (const T &t);
public:
    GretlBundle (void);                             // default constructor with empty bundle
    GretlBundle (gretl_bundle *src);                // incoming already existing bundle Note: src must be deallocated by libgretl
    GretlBundle (MODEL *model, DATASET *dset);      // we produce bundle from MODEL
    GretlBundle (GRETL_VAR *model, DATASET *dset);  // we produce bundle from VAR/VECM
    GretlBundle (equation_system *model);           // we produce bundle from SYSTEM
    GretlBundle (const py::kwargs &kwargs);         // for on-the-fly initialization
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

    void delete_element (const std::string &key);

    py::object pop (const std::string &key);
};

//
// Class to handle 'gretl_matrix' type
//
class GretlMatrix
{
private:
    gretl_matrix *m_ = nullptr;                                             // libgretl gretl_matrix raw pointer
    mutable GretlPrint printer_ = GretlPrint();                             // printer (instance wide)
    mutable int err_ = 0;

    void check_bounds_ (int i, int j) const;                                // we check if given [i,j] are valid
    void numpy_to_matrix_ (const py::array &array);                         // helper for NumPy to GretlMatrix
public:
    GretlMatrix (void);                                                     // default constructor
    GretlMatrix (int rows, int cols, double fill, const std::string &type); // filled real/complex matrix
    GretlMatrix (int rows, int cols, const std::string &dist);              // random matrix
    GretlMatrix (double start, double end, double step = 1.0);              // sequential row vector
    GretlMatrix (int n);                                                    // identity matrix
    GretlMatrix (const py::array &array);                                   // NumPy to GretlMatrix
    GretlMatrix (const gretl_matrix *src);                                  // gretl_matrix to GretlMatrix
    GretlMatrix (const py::list &list);                                     // Python list to GretlMatrix
    ~GretlMatrix (void);

    gretl_matrix *get_ptr (void) const;                                     // raw pointer to m_

    double *val (void) const;                                               // raw pointer to double storage

    double _Complex *z (void) const;                                        // raw pointer to complex storage

    int rows (void) const;

    int cols (void) const;

    [[nodiscard]] bool is_complex (void) const noexcept;

    std::string repr_impl (void);                                           // gretl_matrix printing via __repr__

    template <bool ROWS>
    py::list rows_cols_names_get (void) const;

    template <bool ROWS>
    void rows_cols_names_set (const py::list &src) const;

    py::object get_element (int i, int j) const;

    template <typename T>
    void set_element (int i, int j, T val);

    py::object to_numpy (void) const;
};

#endif // GRETL4PY_DATA_H
