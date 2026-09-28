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

#ifndef GRETL4PY_DATASET_H
#define GRETL4PY_DATASET_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "gretl4py_types.h"
#include "gretl_calls.h"
#include <gretl/gretl_matrix.h>
extern "C" {
    #include <gretl/gretl_func.h>
}

namespace py = pybind11;

template<typename T>
inline constexpr bool is_gretllist_arg_v = std::is_same_v<T, std::string> || std::is_same_v<T, py::list>;

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

    GretlDatasetPtr exec_slice_ (GretlSlice slice_params);
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

    void print (const py::list &varlist, const py::kwargs &kwargs);

    void set_as_default (void);

    // exporters
    py::object get_accessor (const std::string &name);

    py::dict to_dict (void) const;

    template <typename T>
    void to_file (const std::string &filename, const T &varlist, const py::kwargs &kwargs);

    template <typename T>
    GretlMatrixPtr to_matrix (const T &list, int startobs = 0, int endobs = 0, const std::string &missing = "ok");

    // dataset information
    [[nodiscard]] bool is_panel (void) const noexcept;

    [[nodiscard]] bool is_time_series (void) const noexcept;

    [[nodiscard]] bool is_cross_section (void) const noexcept;

    py::list varnames (void) const;

    int get_series_id (const std::string &series, const bool verbose = true) const;

    py::list get_obs_labels (void) const;

    std::string summary (bool print = true) const noexcept;

    // dataset management
    void setobs (const std::string &periodicity, const std::string &startobs, const py::kwargs &kwargs);

    void smpl (const std::string &startobs, const std::string &endobs, const py::kwargs &kwargs);

    GretlDatasetPtr getitem_impl (const py::slice &slice);

    GretlDatasetPtr getitem_impl (const std::string &slice);

    GretlDatasetPtr getitem_impl (const py::list &slice);

    void gretl_series_set_label (const int number, char *label);

    char *add_series (gretl_double_ptr data, const std::string &name, int n, gretl_gchar_ptr label = nullptr);

    void add_series (const py::array &data, const std::string &name);

    void add_series (const GretlMatrix &data, const std::string &name);

    void new_series (const std::string &formula);

    void new_series (const std::string &name, const std::string &value);

    void new_list (const std::string &formula);

    void new_list (const std::string &name, const std::string &value);

    void rename (int series_id, const std::string &newname, const py::kwargs &kwargs);

    GretlMatrixPtr get_series (const std::string &series) const;

    gretl_list_ptr gretllist (const std::string &formula, int *err = nullptr);

    gretl_list_ptr gretllist (const py::list &list, int *err = nullptr);

    std::string python_list_to_formula (const py::list &list) const;

    // tests
    GretlBundlePtr test (const std::string &type, const std::string &varlist, const py::kwargs &kwargs);

    // transformations
    template <typename T, GretlCmdIndex CI>
    void transforms (const T &varlist, const py::kwargs &kwargs);

    template <typename T, GretlCmdIndex CI>
    void transforms_no_opts (const T &varlist);

    template <typename T>
    void lags (const T &varlist, int order);

    template <typename T>
    void do_modify_dataset (DatasetOp op, const T &list, const std::string &param, const py::kwargs &kwargs);

    GretlMatrixPtr tdisagg (py::object Y, int s, const py::kwargs &kwargs);

    // filters, correlations, etc
    GretlMatrixPtr hpfilt (const std::string &y, double hp_lambda = NADBL, bool trend = false, bool one_sided = false);

    GretlMatrixPtr bwfilt (const std::string &y, int n, double omega);

    GretlMatrixPtr bkfilt (const std::string &y, int f1 = 0, int f2 = 0, int k = 0);

    template <typename T>
    py::object corr (const T &varlist, const py::kwargs &kwargs);

    template <typename T>
    GretlMatrixPtr cov (const T &varlist);

    py::object hurst (const std::string &series);

    template <typename T>
    py::object xcorrgm (const T &varlist, int order, const py::kwargs &kwargs);

    template <typename T>
    py::object pca (const T &varlist, const py::kwargs &kwargs);

    template <typename T>
    GretlBundlePtr kmeans (const T &varlist, int centers, const GretlBundlePtr opts = nullptr);

    template <typename T>
    GretlBundlePtr kmeans (const T &varlist, const GretlMatrix &centers, const GretlBundlePtr opts = nullptr);

    template <typename T>
    py::object xtab (const T &varlist, const py::kwargs &kwargs);
};

#endif // GRETL4PY_DATASET_H
