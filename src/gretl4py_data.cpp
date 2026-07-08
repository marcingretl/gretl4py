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
#include "gretl4py_data.h"
#include "gretl4py_types.h"
#include "gretl4py_model.h"
#include "gretl4py_packages.h"
#include "gretl_calls.h"
#include "gretl4py_utils.h"
#include <gretl/libset.h>
#include <gretl/gretl_matrix.h>
#include <gretl/matrix_extra.h>
#include <gretl/gretl_model.h>
#include <gretl/gretl_drivers.h>
extern "C" {
    #include <gretl/var.h>
    #include <gretl/gretl_xml.h>
    #include <gretl/dataset.h>
    #include <gretl/system.h>
    #include <gretl/uservar.h>
    // this is because we cannot include gretl_cmatrix.h
    int gretl_cmatrix_print (const gretl_matrix *A, const char *name, PRN *prn);
    gretl_matrix *gretl_cmatrix_multiply (const gretl_matrix *A, const gretl_matrix *B, int *err);
    gretl_matrix *gretl_cmatrix_divide (const gretl_matrix *A, const gretl_matrix *B, GretlMatrixMod mod, int *err);
    gretl_matrix *gretl_cmatrix_add_sub (const gretl_matrix *A, const gretl_matrix *B, int sgn, int *err);
    gretl_matrix *gretl_cmatrix_dot_op (const gretl_matrix *a, const gretl_matrix *b, int op, int *err);
}
#include <filesystem>
#include <any>

//////////////////////////////////////////////////
// static functions
//////////////////////////////////////////////////
// T == source type
template <typename T>
static void real_NumPyArray_to_gretl_storage (const NumPyArrayView &view, double *dst, int offset, bool onecol)
{
    //
    // Convert a NumPy array (T) to gretl_matrix (double*).
    // - offset: column offset (used when copying a single column)
    // - onecol: true if we're copying one column only
    //
    const T *src = static_cast<const T*>(view.data);
    const bool fast_path = view.NumPyArrayFC && std::is_same_v<T, double>;

    if (onecol) {
        if (fast_path) {
            std::memcpy(dst, src + offset * view.rows, view.rows * sizeof(double));
        } else {
            for (int i=0; i<view.rows; ++i) {
                dst[i] = static_cast<double>(src[i * view.row_stride + offset * view.col_stride]);
            }
        }
    } else {
        if (fast_path) {
            std::memcpy(dst, src, view.rows * view.cols * sizeof(double));
        } else {
            for (int j=0; j<view.cols; ++j) {
                for (int i=0; i<view.rows; ++i) {
                    dst[j * view.rows + i] = static_cast<double>(src[i * view.row_stride + j * view.col_stride]);
                }
            }
        }
    }
}

static void complex_NumPyArray_to_gretl_matrix (const NumPyArrayView &view, gretl_matrix *dst)
{
    //
    // Convert a NumPy std::complex<double> array to gretl_matrix (double _Complex*)
    //
    if (!dst->is_complex) throw std::invalid_argument("matrix must be complex");
    const auto *src = static_cast<const std::complex<double>*>(view.data);

    for (int j=0; j<view.cols; ++j) {
        for (int i=0; i<view.rows; ++i) {
            const auto val = src[i * view.row_stride + j * view.col_stride];
            double _Complex z = { val.real(), val.imag() };

            gretl_cmatrix_set(dst, i, j, z);
        }
    }
}

static inline std::complex<double> c_complex_to_std_complex (const double _Complex z) { return {__real__ z, __imag__ z}; }

static inline double _Complex std_complex_to_c_complex (std::complex<double> z) { return {z.real(), z.imag()}; }

/*
static inline bool has_negative_base (const gretl_matrix *m)
{
    for (int i=0; i<m->rows * m->cols; ++i) {
        double x = m->val[i];

        if ((std::isfinite)(x) && x < 0.0) return true;
    }

    return false;
}

static inline bool has_non_integer_exponent (double x)
{
    return !(std::isfinite)(x) || std::floor(x) != x;
}

static inline bool has_non_integer_exponent (const gretl_matrix *m)
{
    for (int i=0; i<m->rows * m->cols; ++i) {
        double x = m->val[i];

        if (!(std::isfinite)(x) || std::floor(x) != x) return true;
    }

    return false;
}
*/

static inline GretlSlice make_gretl_slice (std::string startobs, std::string endobs,
                                           std::string slice_str, py::dict opts = {})
{
    //
    // small helper for fast GretlSlice generation
    //
    GretlSlice gs;

    gs.startobs = std::move(startobs);
    gs.endobs = std::move(endobs);
    gs.slice_str = std::move(slice_str);
    gs.opts = std::move(opts);

    return gs;
}

static GretlSlice python_slice_to_gretl_slice (const py::slice &slice)
{
    //
    // we convert Python's slice string accepted by libgretl
    //
    py::object start = slice.attr("start");
    py::object stop = slice.attr("stop");
    std::string startobs = is_pynone(start) ? "1" : std::to_string(start.cast<int>() + 1); // this is to get 0-based indexing
    std::string endobs = is_pynone(stop) ? "end" : std::to_string(stop.cast<int>());
    std::string slice_str = startobs + ":" + endobs;

    return make_gretl_slice(startobs, endobs, slice_str);
}

static GretlSlice python_slice_to_gretl_slice (const std::string &slice)
{
    //
    // we manufacture string accepted by libgretl
    //
    std::string startobs, endobs;
    py::dict opts;
    auto pos = slice.find(':');

    if (pos != std::string::npos && slice.find(':', pos + 1) == std::string::npos) {
        startobs = slice.substr(0, pos);
        endobs   = slice.substr(pos + 1);
    } else {
        startobs = slice;
        endobs   = "";
        opts["restrict"] = true;
    }

    return make_gretl_slice(startobs, endobs, slice, opts);
}

static std::string gretl_slice_from_int (int val)
{
    //
    // here we manufacture gretl slicing string based on given int value (including negative ones)
    //
    std::string slice_str;

    if (val == -1) {
        slice_str = "end";
    } else if (val < -1) {
        slice_str = "end" + std::to_string(val + 1); // this is to get 0-based indexing
    } else {
        slice_str = std::to_string(val + 1);
    }

    return slice_str;
}

//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////
template <typename T>
void NumPyArray_to_gretl_storage (NumPyArrayView &view, T *dst, int offset, bool onecol)
{
    //
    // here we copy contents of some NumPy array described by &view
    // to storage available at *dst
    // offset here is used when we copy to DATASET
    //
    double *storage = nullptr;

    if constexpr (std::is_same_v<T, gretl_matrix>) {
        storage = dst->val;
    } else {
        storage = dst;
    }

    if (view.is_double) {
        real_NumPyArray_to_gretl_storage<double>(view, storage, offset, onecol);
    } else if (view.dtype.is(py::dtype::of<float>())) {
        real_NumPyArray_to_gretl_storage<float>(view, storage, offset, onecol);
    } else if (view.dtype.is(py::dtype::of<int>())) {
        real_NumPyArray_to_gretl_storage<int>(view, storage, offset, onecol);
    } else if (view.dtype.is(py::dtype::of<int64_t>())) {
        real_NumPyArray_to_gretl_storage<int64_t>(view, storage, offset, onecol);
    } else if (view.is_complex) {
        if constexpr (std::is_same_v<T, gretl_matrix>) {
            complex_NumPyArray_to_gretl_matrix(view, reinterpret_cast<gretl_matrix*>(dst));
        } else {
            throw std::runtime_error("Complex NumPy -> non-complex destination not supported");
        }
    } else {
        throw std::runtime_error("Unsupported NumPy dtype for conversion.");
    }
}
template void NumPyArray_to_gretl_storage<gretl_matrix>(NumPyArrayView&, gretl_matrix*, int, bool);

//////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////
//
//GretlDataset class methods
//
GretlDataset::GretlDataset (const std::string &fname, const py::dict &kwargs) : id_(next_obj_id_++), from_file_(true)
{
    //
    // load gretl data from file
    //
    std::string full_path = "";
    bool quiet = false;

    // we look for 'frompkg' option
    if (py::len(kwargs) > 0) {
        for (const auto &[key, val] : kwargs) {
            auto key_str = key.cast<std::string>();

            if (key_str == "frompkg") {
                if (!is_pystr(val)) throw std::invalid_argument("'frompkg' value must be a string");
                auto pkg = GretlPackage::load_package(val.cast<std::string>());

                full_path = pkg->get_examples_path(fname).string();
            } else if (key_str == "quiet") {
                quiet = is_bool_kwarg(val);
            } else {
                throw std::invalid_argument("unsupported option: '" + key_str + "'");
            }
        }
    }

    allocate_dset_();
    if (!dset) handle_gretl_error(E_ALLOC);

    // we try to read the file
    err_ = gretl_get_data(full_path.empty() ? fname.c_str() : full_path.c_str(), dset, OPT_NONE, printer_.prn_ptr());
    if (!quiet || err_) printer_.print();
    if (err_) {
        destroy_dataset(dset);
        handle_gretl_error(err_, full_path.empty() ? fname : full_path);
    }

    // we try to read dataset description
    if (full_path.empty()) {
        char buf[MAXLEN] = {0};

        strncpy(buf, fname.c_str(), MAXLEN-1);
        if (auto ptr = gretl_addpath(buf, 0)) source = std::string(ptr);
    } else {
        source = full_path;
    }

    // we set obs labels
    set_obs_labels_();
}

GretlDataset::GretlDataset (const py::dict &pydict) : id_(next_obj_id_++)
{
    //
    // here we fill a gretl DATASET with the content of @pydict
    //
    py::list pylist;
    py::array_t<double> pyarray;
    NumPyArrayView arr_view;

    // lambda for getting int from pydict
    auto get_int_from_pydict = [&pydict](const char *key, int deflt) -> int
    {
        int ret = deflt;

        if (pydict.contains(key)) {
            auto element = pydict[py::cast(key)];

            if (is_pyint(element)) ret = py::cast<int>(element);
        }

        return ret;
    };

    // lambda for getting opt from data structure
    auto setobs_opt_from_structure = [](int structure) -> gretlopt
    {
        if (structure == CROSS_SECTION) {
            return OPT_X;
        } else if (structure == TIME_SERIES) {
            return OPT_T;
        } else if (structure == STACKED_TIME_SERIES) {
            return OPT_S;
        } else if (structure == SPECIAL_TIME_SERIES) {
            return OPT_N;
        } else {
            return OPT_NONE;
        }
    };

    // we allocate memory
    allocate_dset_();

    // we fill up dset->Z
    get_object_from_dict<py::array_t<double>>(pydict, "data_array", pyarray);
    NumPyArray_extract(pyarray, arr_view);
    if (!arr_view.data) throw std::runtime_error("NumPy array: null pointer.");
    dset->n = arr_view.rows;
    dset->v = arr_view.cols + 1;
    err_ = allocate_Z(dset, OPT_NONE);
    if (err_) handle_gretl_error(err_);
    for (int i=1; i<dset->v; i++) {
        NumPyArray_to_gretl_storage<double>(arr_view, dset->Z[i], i-1, true);
    }
    dset->t1 = 0;
    dset->t2 = dset->n-1;

    // we allocate memory for varnames and obs markers
    if (dataset_allocate_varnames(dset) || dataset_allocate_obs_markers(dset)) handle_gretl_error(E_ALLOC);

    // we fill up dset->S
    pylist = pydict[py::cast("obs_labels")];
    if (pylist.size() != static_cast<long unsigned int>(dset->n)) throw std::invalid_argument("Number of obs_labels is different than number of observations.");
    for (int i=0; i<dset->n; ++i) {
        std::string el = py::str(pylist[i]);

        std::memset(dset->S[i], '\0', OBSLEN);
        el.copy(dset->S[i], OBSLEN, 0);
    }

    // we fill up dset->varname
    pylist = pydict[py::cast("varname")];
    if (pylist.size() != static_cast<long unsigned int>(dset->v-1)) throw std::invalid_argument("Number of varnames is different than number of variables.");
    for (int i=0; i<dset->v-1; i++) {
        if (!is_pystr(pylist[i])) throw std::runtime_error("name of variable " + std::to_string(i) + " not a string");
        auto el = py::cast<std::string>(pylist[i]);

        std::memset(dset->varname[i+1], '\0', VNAMELEN);
        gretl_normalize_varname(dset->varname[i+1], el.c_str(), 1, 0);
    }

    // The following actions work fine if the python dict we're working with
    // was created by GretlDataset::to_dict (that is, starting from a gretl
    // dataset in libgretl's memory-space).
    dset->structure = get_int_from_pydict("structure", 0);
    set_obs(std::to_string(get_int_from_pydict("periodicity", 1)).c_str(), dset->S[0], dset, setobs_opt_from_structure(dset->structure));

    // we set obs labels
    set_obs_labels_();
}

GretlDataset::GretlDataset (const GretlDataset &src) : id_(next_obj_id_++)
{
    //
    // copy constructor
    //
    if (!src.dset) handle_gretl_error(E_DATA);

    allocate_dset_();
    if (!dset) handle_gretl_error(E_ALLOC);

    // simple types
    dset->v = src.dset->v;
    dset->n = src.dset->n;
    dset->pd = src.dset->pd;
    dset->structure = src.dset->structure;
    dset->sd0 = src.dset->sd0;
    dset->t1 = src.dset->t1;
    dset->t2 = src.dset->t2;
    dset->markers = src.dset->markers;
    dset->rseed = src.dset->rseed;
    dset->auxiliary = src.dset->auxiliary;
    dset->n_varinfo = src.dset->n_varinfo;
    dset->panel_pd = src.dset->panel_pd;
    dset->panel_sd0 = src.dset->panel_sd0;
    std::strncpy(dset->stobs, src.dset->stobs, OBSLEN);
    std::strncpy(dset->endobs, src.dset->endobs, OBSLEN);

    // we allocate memory
    err_ = allocate_Z(dset, OPT_NONE);
    if (err_) handle_gretl_error(err_);
    err_ = dataset_allocate_varnames(dset);
    if (err_) handle_gretl_error(err_);
    err_ = dataset_allocate_obs_markers(dset);
    if (err_) handle_gretl_error(err_);

    // dset->Z
    for (int i=1; i<dset->v; ++i) {
        std::memcpy(dset->Z[i], src.dset->Z[i], src.dset->n * sizeof(double));
    }

    // dset->varname and dset->VARINFO
    for (int i=0; i<src.dset->v-1; ++i) {
        std::memset(dset->varname[i+1], '\0', VNAMELEN);
        gretl_normalize_varname(dset->varname[i+1], src.dset->varname[i+1], 1, 0);
        copy_varinfo(dset->varinfo[i], src.dset->varinfo[i]);
    }

    // dset->S
    copy_dataset_obs_info(dset, src.dset);

    // simple strings
    if (src.dset->descrip)     dset->descrip     = strdup(src.dset->descrip);
    if (src.dset->submask)     dset->submask     = strdup(src.dset->submask);
    if (src.dset->restriction) dset->restriction = strdup(src.dset->restriction);
    if (src.dset->padmask)     dset->padmask     = strdup(src.dset->padmask);
    if (src.dset->mapfile)     dset->mapfile     = strdup(src.dset->mapfile);
    if (src.dset->pangrps)     dset->pangrps     = strdup(src.dset->pangrps);

    // we set obs labels
    set_obs_labels_();

    // we mark it as copy of ...
    if (!src.source.empty()) {
        source = src.source + " (copy of " + std::to_string(src.get_id()) + ")";
    } else {
        source = "copy of dataset " + std::to_string(src.get_id());
    }
}

GretlDataset::~GretlDataset (void)
{
    if (is_default) default_dataset_.reset();
    dataset_.erase(id_);
    free_dset_();
}

GretlDatasetPtr GretlDataset::get_dset_by_id_ (int id)
{
    //
    // we return pointer to already registered dataset
    //
    auto dset_reg = get_dset_ids();

    if (!dset_reg.count(id)) throw std::invalid_argument("there is no dataset with id = " + std::to_string(id));

    auto it = dataset_.find(id);

    return it->second.lock();
}

void GretlDataset::register_dset_ (const GretlDatasetPtr dataset )
{
    //
    // here we register newly created GretlDataset instance
    //
    if (!dataset) throw std::runtime_error("Attempt to register null dataset");

    auto [it, ok] = dataset_.insert({dataset->get_id(), dataset});

    if (!ok) throw std::logic_error("Duplicate dataset id: " + std::to_string(dataset->get_id()));
}

GretlDatasetPtr GretlDataset::clone (const GretlDataset &src)
{
    //
    // factory method for copy constructor
    //
    auto dataset = GretlDatasetPtr(new GretlDataset(src));

    // we register dataset
    register_dset_(dataset);

    return dataset;
}

GretlDatasetPtr GretlDataset::get_data (const py::object &src, const std::string &source, const py::kwargs &kwargs)
{
    //
    // main factory method for getting data (loading from src or registry)
    // defined to avoid binding the class constructor on the Python side
    //
    GretlDatasetPtr dataset;
    bool register_dataset = true;

    if (is_pystr(src)) {
        dataset = GretlDatasetPtr(new GretlDataset(src.cast<std::string>(), kwargs));
    } else if (is_pydict(src)) {
        dataset = GretlDatasetPtr(new GretlDataset(src.cast<py::dict>()));
    } else if (is_pyint(src)) {
        dataset = get_dset_by_id_(src.cast<int>());
        register_dataset = false;
    } else {
      throw std::invalid_argument("GretlDataset::get_data(): unsupported src type");
    }
    if (!dataset) handle_gretl_error(E_DATA);

    // we set default (id needed)
    if (!default_dataset_.lock()) dataset->set_as_default();

    // we set info
    if (!source.empty() && dataset->source.empty()) dataset->source = source;

    // we register dataset
    if (register_dataset) register_dset_(dataset);

    return dataset;
}

std::set<int> GretlDataset::get_dset_ids (void)
{
    //
    // here we return a set of alive dataset's ids
    // (i.e. we erase already deleted)
    //
    std::set<int> res;

    for (auto it = dataset_.begin(); it != dataset_.end();) {
        // at first we always erase already deleted datasets
        if (it->second.expired()) {
            it = dataset_.erase(it);
        } else {
            res.insert(it->first);
            ++it;
        }
    }

    return res;
}

GretlDatasetPtr GretlDataset::default_dataset (void)
{
    return default_dataset_.lock();
}

void GretlDataset::allocate_dset_ (void)
{
    free_dset_();
    dset = datainfo_new();
}

void GretlDataset::free_dset_ (void)
{
    if (dset) {
        destroy_dataset(dset);
        dset = nullptr;
    }
}

void GretlDataset::add_or_replace_allocated_series_ (gretl_double_ptr data, const std::string &name, int n, gretl_gchar_ptr label)
{
    //
    // Here we add or replace allocated series to dataset
    // Note: following libgretl convension, data is grabbed
    // only if there is no error
    //
    // we do basic checks
    if (n != dset->n) throw std::invalid_argument("'" + name + "' is of wrong length: is n=" + std::to_string(n) + ", should be n=" + std::to_string(dset->n));
    if (name.empty()) throw std::invalid_argument("empty variable name");

    // we check if we add new or replace series
    int v = get_series_id(name, false);

    // actual work
    double *data_ptr = data.release();

    if (v > 0) {
        err_ = dataset_replace_series(dset, v, data_ptr, label.get(), DS_GRAB_VALUES);
    } else {
        err_ = dataset_add_allocated_series(dset, data_ptr);
    }
    if (err_) {
        std::free(data_ptr);
        handle_gretl_error(err_);
    }

    if (v > 0) {
        py::print("Replaced series " + name + " (ID " + std::to_string(v) + ")");
    } else {
        // we setup name and description for new series
        v = dset->v-1;
        err_ = rename_series(dset, v, name.c_str(), OPT_NONE);
        if (err_) handle_gretl_error(err_);
        gretl_series_set_label(v, label.get());
    }
}

void GretlDataset::set_obs_labels_ (void)
{
    //
    // we set obs labels
    //
    char s[OBSLEN];

    obs_labels_.resize(dset->n);
    for (int i=0; i<=dset->n-1; ++i) {
        ntolabel(s, i, dset);
        obs_labels_[i] = s;
    }
}

GretlDatasetPtr GretlDataset::exec_slice_ (GretlSlice slice_params)
{
    //
    // here we execute slicing via smpl() and perma_sample() (for py::slice or std::string)
    //
    auto res = GretlDatasetPtr(new GretlDataset(*this));
    auto opts = slice_params.opts;

    opts["quiet"] = true;
    res->smpl(slice_params.startobs, slice_params.endobs, opts);
    err_ = perma_sample(res->dset, OPT_T, printer_.prn_ptr(), nullptr);
    if (err_) {
        printer_.print();
        handle_gretl_error(err_);
    }

    return res;
}

int GretlDataset::get_series_id (const std::string &series, const bool verbose) const
{
    //
    // method returns id of given series (in dset context)
    //
    int var_num = -1;

    var_num = current_series_index(dset, series.c_str());
    if (var_num < 0 && verbose) throw std::invalid_argument("'" + series + "': series does not exist");

    return var_num;
}

py::list GretlDataset::get_obs_labels (void) const
{
    return py::cast(obs_labels_);
}

bool GretlDataset::is_panel (void) const
{
    return static_cast<bool>(dataset_is_panel(dset));
}

std::string GretlDataset::summary (bool print) const
{
    //
    // method prints basic info about dataset
    //
    auto *prn = printer_.prn_ptr();

    if (dataset_is_time_series(dset)) {
        pprintf(prn, "Time series, frequency %d, T = %d, %s to %s\n\n", dset->pd, dset->n, obs_labels_.at(0).c_str(), obs_labels_.back().c_str());
    } else if (is_panel()) {
        pprintf(prn, "Panel data, N = %d, T = %d\n\n", dset->n / dset->pd, dset->pd);
    } else {
        pprintf(prn, "Cross-sectional data, N = %d\n\n", dset->n);
    }
    list_series(dset, OPT_NONE, prn);
    if (print) printer_.print();

    return printer_.get_buffer();
}

py::dict GretlDataset::to_dict (void) const
{
    //
    // we convert gretl data to pydict
    //
    py::dict as_dict;
    py::list vnames, labels;
    double *tmp = nullptr;
    int i = 0;

    as_dict[py::cast("periodicity")] = dset->pd;
    as_dict[py::cast("structure")] = dset->structure;

    // note 2024-11-20: we don't put 'const' into the python dictionary,
    // so we skip series 0 when copying from GretlDataset.dset

    // copy across the data values
    DNumPyArrayFC Z({dset->n, dset->v-1});
    tmp = Z.mutable_data();
    for (i=1; i<dset->v; i++) {
        std::memcpy(tmp, dset->Z[i], dset->n * sizeof *tmp);
        tmp += dset->n;
    }
    as_dict[py::cast("data_array")] = Z;

    // copy across the series names
    for (i=1; i<dset->v; i++) {
        vnames.append(dset->varname[i]);
    }
    as_dict[py::cast("varname")] = vnames;

    // copy or manufacture obs labels
    if (dset->S != nullptr) {
        for (i=0; i<dset->n; i++) {
            labels.append(dset->S[i]);
        }
    } else {
        char s[OBSLEN];

        for (i=0; i<dset->n; i++) {
            labels.append(ntolabel(s, i, dset));
        }
    }
    as_dict[py::cast("obs_labels")] = labels;

    return as_dict;
}

void GretlDataset::to_file (const std::string &filename, const std::string &varlist,
                            const py::kwargs &kwargs)
{
    //
    // methods which mimics libgretl's STORE command
    //
    gretl_list_ptr var_list{nullptr};
    std::nullptr_t fake_dataset = nullptr;

    // we handle varlist
    if (!varlist.empty()) var_list = gretllist(varlist);

    // we unpack kwargs;
    opt_ = unpack_kwargs_command(STORE, fake_dataset, kwargs);

    // we do the job
    if (opt_ & OPT_A) {
        err_ = write_matrix_as_dataset(filename.c_str(), opt_, printer_.prn_ptr());
    } else {
        err_ = write_data(filename.c_str(), var_list.get(), dset, opt_, printer_.prn_ptr());
    }
    if (err_) handle_gretl_error(err_);
    printer_.print();
}

template <typename T>
GretlMatrixPtr GretlDataset::to_matrix (const T &list, int startobs, int endobs, const std::string &missing)
{
    //
    // here we construct GretlMatrix from dataset based on given list
    //
    GretlMatrixPtr res{nullptr};
    gretl_list_ptr gretl_list{nullptr};
    MMissingCode missop_code;

    // we construct list of variables
    if (list.empty()) {
        gretl_list.reset(full_var_list(dset, nullptr));
    } else {
        if constexpr (is_gretllist_arg_v<T>) {
            gretl_list = gretllist(list);
        } else {
            static_assert(always_false<T>, "GretlDataset::to_matrix(): unsupported type");
        }
    }

    if (startobs && startobs < 1) throw std::invalid_argument("startobs cannot be < 1");
    if (endobs && endobs > dset->n) throw std::invalid_argument("endobs cannot greater than total number of observations");
    if (missing == "ok") {
        missop_code = M_MISSING_OK;
    } else if (missing == "error") {
        missop_code = M_MISSING_ERROR;
    } else if (missing == "skip") {
        missop_code = M_MISSING_SKIP;
    } else {
        throw std::invalid_argument("unknown value for missing");
    }
    res = std::make_shared<GretlMatrix>(gretl_matrix_data_subset(gretl_list.get(), dset,
                                                                 startobs ? startobs : dset->t1,
                                                                 endobs ? endobs : dset->t2, missop_code,
                                                                 &err_), false);
    if (err_) handle_gretl_error(err_);

    return res;
}
template GretlMatrixPtr GretlDataset::to_matrix<std::string>(const std::string &list, int startobs, int endobs, const std::string &missing);
template GretlMatrixPtr GretlDataset::to_matrix<py::list>(const py::list &list, int startobs, int endobs, const std::string &missing);

char *GretlDataset::add_series (gretl_double_ptr data, const std::string &name,
                                int n, gretl_gchar_ptr label)
{
    //
    // Here we add existing series to dataset
    // Note: gretl_double_ptr and gretl_gchar_ptr are grabbed by dset
    //
    add_or_replace_allocated_series_(std::move(data), name, n, std::move(label));

    return dset->varname[get_series_id(name, false)];
}

void GretlDataset::add_series (const py::array &data, const std::string &name)
{
    //
    // Here we add existing py::array to dataset
    // Note: the underline data is copied (deep copy)
    //
    NumPyArrayView view;

    NumPyArray_extract(data, view);
    if ((std::min)(view.cols, view.rows) > 1) throw std::invalid_argument("given variable is not a vector");
    int n = (std::max)(view.cols, view.rows);

    // we use std::malloc since memory will be deallocated by libgretl
    gretl_double_ptr data_copy(static_cast<double*>(std::malloc(n * sizeof(double))));

    // actual work
    NumPyArray_to_gretl_storage<double>(view, data_copy.get(), 0, true);
    add_or_replace_allocated_series_(std::move(data_copy), name, n);
}

void GretlDataset::add_series (const GretlMatrix &data, const std::string &name)
{
    //
    // Here we add existing GretlMatrix to dataset
    //

    if ((std::min)(data.cols(), data.rows()) > 1) throw std::invalid_argument("given variable is not a vector");
    if (data.is_complex()) throw std::invalid_argument("given matrix is complex, must be real");
    int n = (std::max)(data.cols(), data.rows());

    // we use std::malloc since memory will be deallocated by libgretl
    gretl_double_ptr data_copy(static_cast<double*>(std::malloc(n * sizeof(double))));

    // actual work
    std::memcpy(data_copy.get(), data.val(), n * sizeof(double));
    add_or_replace_allocated_series_(std::move(data_copy), name, n);
}

void GretlDataset::new_series (const std::string &formula)
{
    // new series according to @formula
    gretl_generate<GRETL_TYPE_SERIES>(formula, shared_from_this());
}

void GretlDataset::new_series (const std::string &name, const std::string &value)
{
    // new series according to @name and @value
    gretl_generate<GRETL_TYPE_SERIES>(value, shared_from_this(), name);
}

void GretlDataset::new_list (const std::string &formula)
{
    // new list according to @formula
    gretl_generate<GRETL_TYPE_LIST>(formula, shared_from_this());
}

void GretlDataset::new_list (const std::string &name, const std::string &value)
{
    // new list according to @name and @value
    gretl_generate<GRETL_TYPE_LIST>(value, shared_from_this(), name);
}

void GretlDataset::rename (int series_id, const std::string &newname, const py::kwargs &kwargs)
{
    //
    // this method mimics libgretl's 'rename' command
    //
    std::nullptr_t fake_dataset = nullptr;

    opt_ = unpack_kwargs_command(RENAME, fake_dataset, kwargs);

    // we do the job
    err_ = rename_series(dset, series_id, newname.c_str(), opt_);
    if (err_) handle_gretl_error(err_);
}

template <typename T, GretlCmdIndex CI>
void GretlDataset::transforms (const T &varlist, const py::kwargs &kwargs)
{
    //
    // interface to several gretl's commands (CI)
    // which underlying engines live in tranforms.c
    //
    std::nullptr_t fake_dataset = nullptr;
    gretl_list_ptr var_list{nullptr};

    // we unpack kwargs;
    opt_ = unpack_kwargs_command(CI, fake_dataset, kwargs);

    // we construct list of variables
    if constexpr (is_gretllist_arg_v<T>) {
        var_list = gretllist(varlist);
    } else {
        static_assert(always_false<T>, "GretlDataset::transforms(): unsupported var_list type");
    }

    // we do the job
    int *var_list_raw_ptr = var_list.release();

    if constexpr (CI == SQUARE) {
        err_ = list_xpxgenr(&var_list_raw_ptr, dset, opt_);
    } else if constexpr (CI == DUMMIFY) {
        err_ = list_dumgenr(&var_list_raw_ptr, dset, opt_);
    } else if constexpr (CI == DISCRETE) {
        err_ = list_makediscrete(var_list_raw_ptr, dset, opt_);
    } else if constexpr (CI == STDIZE) {
        err_ = list_stdgenr(var_list_raw_ptr, dset, opt_);
    }
    var_list.reset(var_list_raw_ptr);
    if (err_) handle_gretl_error(err_);

    // based on interact.c
    maybe_list_series(dset, printer_.prn_ptr());
    set_dataset_is_changed(dset, 1);

    printer_.print();
}
template void GretlDataset::transforms<std::string, SQUARE>(const std::string&, const py::kwargs&);
template void GretlDataset::transforms<std::string, DUMMIFY>(const std::string&, const py::kwargs&);
template void GretlDataset::transforms<std::string, DISCRETE>(const std::string&, const py::kwargs&);
template void GretlDataset::transforms<std::string, STDIZE>(const std::string&, const py::kwargs&);
template void GretlDataset::transforms<py::list, SQUARE>(const py::list&, const py::kwargs&);
template void GretlDataset::transforms<py::list, DUMMIFY>(const py::list&, const py::kwargs&);
template void GretlDataset::transforms<py::list, DISCRETE>(const py::list&, const py::kwargs&);
template void GretlDataset::transforms<py::list, STDIZE>(const py::list&, const py::kwargs&);

template <typename T, GretlCmdIndex CI>
void GretlDataset::transforms_no_opts (const T &varlist)
{
    //
    // interface to several gretl's commands (CI)
    // which underlying engines live in tranforms.c
    // Note: here we deal with commands which do not take arguments
    //
    gretl_list_ptr var_list{nullptr};

    // we construct list of external variables
    if constexpr (is_gretllist_arg_v<T>) {
        var_list = gretllist(varlist);
    } else {
        static_assert(always_false<T>, "GretlDataset::transforms_no_opts(): unsupported var_list type");
    }

    // we do the job
    int *varlist_raw_ptr = var_list.release();

    if constexpr (CI == DIFF || CI == LDIFF || CI == SDIFF) {
        err_ = list_diffgenr(varlist_raw_ptr, CI, dset);
    } else if constexpr (CI == LOGS) {
        err_ = list_loggenr(varlist_raw_ptr, dset);
    } else if constexpr (CI == ORTHDEV) {
        err_ = list_orthdev(varlist_raw_ptr, dset);
    }
    var_list.reset(varlist_raw_ptr);
    if (err_) handle_gretl_error(err_);

    // based on interact.c
    maybe_list_series(dset, printer_.prn_ptr());
    set_dataset_is_changed(dset, 1);

    printer_.print();
}
template void GretlDataset::transforms_no_opts<std::string, DIFF>(const std::string&);
template void GretlDataset::transforms_no_opts<std::string, LDIFF>(const std::string&);
template void GretlDataset::transforms_no_opts<std::string, SDIFF>(const std::string&);
template void GretlDataset::transforms_no_opts<std::string, LOGS>(const std::string&);
template void GretlDataset::transforms_no_opts<std::string, ORTHDEV>(const std::string&);
template void GretlDataset::transforms_no_opts<py::list, DIFF>(const py::list&);
template void GretlDataset::transforms_no_opts<py::list, LDIFF>(const py::list&);
template void GretlDataset::transforms_no_opts<py::list, SDIFF>(const py::list&);
template void GretlDataset::transforms_no_opts<py::list, LOGS>(const py::list&);
template void GretlDataset::transforms_no_opts<py::list, ORTHDEV>(const py::list&);

std::string GretlDataset::repr (void) const
{
    // we set source of the dataset
    std::string ret = (from_file_ ? "Read datafile: " : "Loaded: ") + source + "\n";

    // we add summary
    ret += summary(false);

    // if dataset read from file, we try to append dataset info
    if (from_file_ && std::filesystem::exists(source)) {
        gretl_char_ptr desc(gretl_get_gdt_description(source.c_str(), &err_));

        if (!err_ && desc) {
            ret.push_back('\n');
            ret += desc.get();
        }
    }

    return ret;
}

void GretlDataset::print (const std::string &varlist, const py::kwargs &kwargs)
{
    py::dict final_kwargs = kwargs;

    // check for banned options
    if (kwargs.contains("data")) throw std::invalid_argument("'data': option not available");

    // we do the job
    final_kwargs["data"] = shared_from_this(); // we must add itself (dataset) for global printer
    do_gretl_print<GretlDataset>(varlist, final_kwargs);
}

void GretlDataset::smpl (const std::string &startobs, const std::string &endobs, const py::kwargs &kwargs)
{
    //
    // method mimics 'smpl' as close as possible (1:1?)
    // note: startobs and endobs are CMD->param/param2 equivalents
    //
    // lambdas
    auto check_smpl_full = [](gretlopt opt) -> int
    {
        //
        // mimics 'static int check_smpl_full (gretlopt)' from interact.c
        //
        opt = static_cast<gretlopt>(opt ^ OPT_F);

        if (opt & OPT_Q) opt = static_cast<gretlopt>(opt ^ OPT_Q);

        return opt == OPT_NONE ? 0 : E_BADOPT;
    };

    auto smpl_restrict = [](gretlopt opt) -> int
    {
        //
        // mimics 'static int 'smpl_restrict (gretlopt)' from interact.c
        //
        opt = static_cast<gretlopt>(opt & (~OPT_Q));
        opt = static_cast<gretlopt>(opt & (~OPT_T));
        opt = static_cast<gretlopt>(opt & (~OPT_D));

        return opt != OPT_NONE;
    };

    gretl_list_ptr var_list{nullptr};
    std::nullptr_t fake_dataset = nullptr;
    std::string vars;
    bool report = false;
    py::dict kwargs_copy = kwargs;

    // note: we modify kwargs here!!!!
    if (kwargs_copy.contains("varlist")) {
        get_object_from_dict<std::string>(kwargs_copy, "varlist", vars);
        kwargs_copy.attr("pop")("varlist");
        var_list = gretllist(vars);
    }

    // we unpack kwargs;
    opt_ = unpack_kwargs_command(SMPL, fake_dataset, kwargs_copy);

    // we do the job
    if (!dset || dset->n <= 0) {
        err_ = E_NODATA;
    } else if (opt_ & OPT_F) {
        err_ = check_smpl_full(opt_);
        if (!err_) err_ = restore_full_sample(dset, nullptr);
    } else if (opt_ == OPT_T && startobs.empty()) {
        err_ = perma_sample(dset, opt_, printer_.prn_ptr(), nullptr);                                              // --permanent, by itself
    } else if (opt_ & (OPT_U | OPT_X)) {
        err_ = set_panel_sample(startobs.c_str(), endobs.c_str(), opt_, dset, nullptr, printer_.prn_ptr());        // panel data: --unit or --time option
    } else if (smpl_restrict(opt_)) {
        err_ = restrict_sample(startobs.c_str(), var_list.get(), dset, nullptr, opt_, printer_.prn_ptr(), nullptr);// --restrict, --dummy, etc.
    } else if (startobs.empty() && endobs.empty()) {
        print_smpl(dset, get_full_length_n(), OPT_F, printer_.prn_ptr());                                       // no args given: give a report
        //print_smpl(dset, get_full_length_n(), OPT_NONE, GP_.prn_ptr());
        report = true;
    } else {
        err_ = set_sample(startobs.c_str(), endobs.c_str(), dset, opt_);                                        // simple setting of t1, t2 business /
    }
    if (err_) handle_gretl_error(err_);

    if (!report && (opt_ & OPT_Q)) print_smpl(dset, get_full_length_n(), OPT_NONE, printer_.prn_ptr());
    printer_.print();
}

GretlDatasetPtr GretlDataset::getitem_impl (const py::slice &slice)
{
    //
    // slicing via Python's slice'
    //
    auto slice_params = python_slice_to_gretl_slice(slice);
    auto res = exec_slice_(slice_params);

    // we register dataset with new name
    res->source += " slice: " + slice_params.slice_str;
    register_dset_(res);

    return res;
}

GretlDatasetPtr GretlDataset::getitem_impl (const std::string &slice)
{
    //
    // slicing via string
    //
    auto slice_params = python_slice_to_gretl_slice(slice);
    auto res = exec_slice_(slice_params);

    // we register dataset with new name
    res->source += " slice: " + slice_params.slice_str;
    register_dset_(res);

    return res;
}

GretlDatasetPtr GretlDataset::getitem_impl (const py::list &slice)
{
    //
    // slicing via Python's py::list (for dropping variables)
    //
    std::string slice_str;
    gretl_list_ptr drop_list{nullptr};
    auto res = GretlDatasetPtr(new GretlDataset(*this));

    // we parse slice / list / string
    std::ostringstream oss;

    // we construct list of variables to retain
    for (size_t i=0; i<slice.size(); ++i) {
        oss << py::str(slice[i]).cast<std::string>();
        if (i + 1 < slice.size()) oss << " ";
    }
    slice_str = oss.str();
    gretl_list_ptr slice_list = gretllist(slice_str);

    // we construct list of variables to drop
    gretl_list_ptr full_list(full_var_list(dset, nullptr));
    drop_list = gretl_list_ptr(gretl_list_drop(full_list.get(), slice_list.get(), &err_));
    if (err_) handle_gretl_error(err_);

    // we do the job
    err_ = gretl_delete_variables(drop_list.get(), nullptr, OPT_F, res->dset, nullptr, printer_.prn_ptr());   // dropping variables
    if (err_) {
        printer_.print();
        handle_gretl_error(err_);
    }
    res->source += " slice: " + slice_str;

    // we register dataset
    register_dset_(res);

    return res;
}

template <typename T>
py::object GretlDataset::corr (const T &varlist, const py::kwargs &kwargs)
{
    //
    // general interface for computing correlations
    //
    gretl_list_ptr var_list{nullptr};
    std::nullptr_t fake_dataset = nullptr;

    // we construct list of variables (if needed)
    if (!varlist.empty()) {
        if constexpr (is_gretllist_arg_v<T>) {
            var_list = gretllist(varlist);
        } else {
            static_assert(always_false<T>, "GretlDataset::corr(): unsupported type");
        }
    }

    // we unpack kwargs;
    opt_ = unpack_kwargs_command(CORR, fake_dataset, kwargs);

    // we do the job
    if (opt_ & OPT_K) {
        err_ = kendall_tau(var_list.get(), dset, opt_, printer_.prn_ptr());
    } else if (opt_ & OPT_S) {
        err_ = spearman_rho(var_list.get(), dset, opt_, printer_.prn_ptr());
    } else {
        err_ = gretl_corrmx(var_list.get(), dset, opt_, printer_.prn_ptr());
    }
    if (err_) handle_gretl_error(err_);
    printer_.print();

    if (opt_ & (OPT_K | OPT_S)) return py::none();

    return get_accessor("$result");
}
template py::object GretlDataset::corr<std::string>(const std::string&, const py::kwargs&);
template py::object GretlDataset::corr<py::list>(const py::list&, const py::kwargs&);

template <typename T>
py::object GretlDataset::cov (const T &vars)
{
    //
    // we compute covariance for given variables
    //
    gretl_list_ptr var_list{nullptr};
    gretl_matrix_ptr mat{nullptr};

    // we construct list of variables
    if (vars.empty()) {
        var_list.reset(full_var_list(dset, nullptr));
    } else {
        if constexpr (is_gretllist_arg_v<T>) {
            var_list = gretllist(vars);
        } else {
            static_assert(always_false<T>, "GretlDataset::corr(): unsupported type");
        }
    }

    // we do the job
    mat.reset(gretl_covariance_matrix_from_varlist(var_list.get(), dset, nullptr, &err_));
    if (err_) handle_gretl_error(err_);
    auto res = convert_GretlType_to_GretlTypes(GRETL_TYPE_MATRIX, mat.get(), 0, err_);

    if (err_) handle_gretl_error(err_);

    return variant_to_pyobject(std::move(*res));
}
template py::object GretlDataset::cov<std::string>(const std::string&);
template py::object GretlDataset::cov<py::list>(const py::list&);

py::object GretlDataset::hurst (const std::string &series)
{
    //
    // we compute the Hurst exponent for given series
    //
    gretl_list_ptr var_list{nullptr};
    opt_ = OPT_NONE;

    // we switch off gnuplot
    gretl_valid_long_opt(opt_, HURST, "plot", py::handle(), "none");

    // we do the job
    var_list = gretllist(series);
    err_ = hurstplot(var_list.get(), dset, opt_, printer_.prn_ptr());
    if (err_) handle_gretl_error(err_);
    printer_.print();

    return get_accessor("$result");
}

template <typename T>
py::object GretlDataset::xcorrgm (const T &vars, int order, const py::kwargs &kwargs)
{
    //
    // we perform cross-correlogram for vars
    //
    gretl_list_ptr var_list{nullptr};
    std::nullptr_t fake_dataset = nullptr;
    bool silent = false;

    // we unpack kwargs
    opt_ = unpack_kwargs_command(XCORRGM, fake_dataset, kwargs);

    // we construct list of variables
    if constexpr (is_gretllist_arg_v<T>) {
        var_list = gretllist(vars);
    } else {
        static_assert(always_false<T>, "GretlDataset::xcorrgm(): unsupported type");
    }

    // we do the job
    err_ = xcorrgram(var_list.get(), order, dset, opt_, printer_.prn_ptr());
    if (err_) handle_gretl_error(err_);
    if (!silent) printer_.print();

    return get_accessor("$result");
}
template py::object GretlDataset::xcorrgm<std::string>(const std::string&, int, const py::kwargs&);
template py::object GretlDataset::xcorrgm<py::list>(const py::list&, int, const py::kwargs&);

template <typename T>
py::object GretlDataset::pca (const T &varlist, const py::kwargs &kwargs)
{
    //
    // we perform Principal Components Analysis following
    // the libgretl's 'pca' command (i.e., not princomp() function)
    //
    gretl_list_ptr var_list{nullptr};
    std::nullptr_t fake_dataset = nullptr;

    // we construct list of variables
    if (varlist.empty()) {
        var_list.reset(full_var_list(dset, nullptr));
    } else {
        if constexpr (is_gretllist_arg_v<T>) {
            var_list = gretllist(varlist);
        } else {
            static_assert(always_false<T>, "GretlDataset::pca(): unsupported type");
        }
    }

    // we unpack kwargs;
    opt_ = unpack_kwargs_command(PCA, fake_dataset, kwargs);

    // we do the job (based on interact.c)
    VMatrix_ptr cmat{nullptr};
    PRN *prn = printer_.prn_ptr();

    // adding OPT_N ensures a uniform sample
    // for the correlation or covariance matrix
    cmat.reset(corrlist(PCA, var_list.get(), dset, static_cast<gretlopt>(opt_ | OPT_N), &err_));
    if (err_) handle_gretl_error(err_);
    err_ = call_pca_plugin(cmat.get(), dset, opt_, prn);
    if (err_) handle_gretl_error(err_);
    if (static_cast<gretlopt>(opt_ & (OPT_O | OPT_A)) && gretl_messages_on()) pputs(prn, _("Generated principal component series\n")); /* results saved as series */
    printer_.print();

    return get_accessor("$result");
}
template py::object GretlDataset::pca<std::string>(const std::string&, const py::kwargs&);
template py::object GretlDataset::pca<py::list>(const py::list&, const py::kwargs&);

template <typename T>
GretlBundlePtr GretlDataset::kmeans (const T &varlist, int centers, const GretlBundlePtr opts)
{
    //
    // here we perform kmeans analysis
    //
    gretl_list_ptr list(gretllist(varlist));
    auto res = std::make_shared<GretlBundle>(gretl_kmeans(nullptr, list.get(), centers, nullptr,
                                                          opts ? opts->get_ptr() : nullptr, dset,
                                                          printer_.prn_ptr(), &err_), false);
    if (err_) handle_gretl_error(err_);
    printer_.print();

    return res;
}
template GretlBundlePtr GretlDataset::kmeans<std::string>(const std::string&, int, const GretlBundlePtr);
template GretlBundlePtr GretlDataset::kmeans<py::list>(const py::list&, int, const GretlBundlePtr);

template <typename T>
GretlBundlePtr GretlDataset::kmeans (const T &varlist, const GretlMatrix &centers, const GretlBundlePtr opts)
{
    //
    // here we perform kmeans analysis
    //
    gretl_list_ptr list(gretllist(varlist));
    auto res = std::make_shared<GretlBundle>(gretl_kmeans(nullptr, list.get(), 0, centers.get_ptr(),
                                                          opts ? opts->get_ptr() : nullptr, dset,
                                                          printer_.prn_ptr(), &err_), false);
    if (err_) handle_gretl_error(err_);
    printer_.print();

    return res;
}
template GretlBundlePtr GretlDataset::kmeans<std::string>(const std::string&, const GretlMatrix&, const GretlBundlePtr);
template GretlBundlePtr GretlDataset::kmeans<py::list>(const py::list&, const GretlMatrix&, const GretlBundlePtr);

template <typename T>
py::object GretlDataset::xtab (const T &varlist, const py::kwargs &kwargs)
{
    //
    // method mimics 'xtab' command
    //
    gretl_list_ptr var_list{nullptr};
    std::nullptr_t fake_dataset = nullptr;

    // we unpack kwargs;
    opt_ = unpack_kwargs_command(XTAB, fake_dataset, kwargs);

    // we construct list of variables
    if (varlist.empty()) {
        var_list.reset(full_var_list(dset, nullptr));
    } else {
        if constexpr (is_gretllist_arg_v<T>) {
            var_list = gretllist(varlist);
        } else {
            static_assert(always_false<T>, "GretlDataset::corr(): unsupported type");
        }
    }

    // we do the job
    err_ = crosstab(var_list.get(), dset, opt_, printer_.prn_ptr());
    if (err_) handle_gretl_error(err_);
    printer_.print();

    return get_accessor("$result");
}
template py::object GretlDataset::xtab<std::string>(const std::string&, const py::kwargs&);
template py::object GretlDataset::xtab<py::list>(const py::list&, const py::kwargs&);

/*
void GretlDataset::setobs (const py::kwargs &kwargs)
{
    //
    // A method to mimic gretl's 'setobs' command
    //
    enum ArgTag { NONE, PERIODICITY, STARTOBS, UNITVAR, TIMEVAR };
    std::string arg1, arg2;
    int rcount = 0;
    int pcount = 0;

    opt_ = OPT_NONE;
    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::invalid_argument("option key must be a string");
        }
        const std::string akey = item.first.cast<std::string>();
        const std::string val = MapPyToCpp(item.second).to_string();

        if (akey == "periodicity" || akey == "startobs" ||
            akey == "unitvar" || akey == "timevar") {
            ArgTag tag = NONE;

            if (akey == "periodicity") {
                tag = PERIODICITY;
                rcount++;
            } else if (akey == "startobs") {
                tag = STARTOBS;
                rcount++;
            } else if (akey == "unitvar") {
                tag = UNITVAR;
                pcount++;
            } else if (akey == "timevar") {
                tag = TIMEVAR;
                pcount++;
            }

            // choose destination arg
            if (tag == PERIODICITY || tag == UNITVAR) {
                arg1 = val;
            } else {
                arg2 = val;
            }
        } else if (akey == "structure") {
            if (val == "cross-section") {
                opt_ = OPT_X;
            } else if (val == "time-series") {
                opt_ = OPT_T;
            } else if (val == "stacked-time-series") {
                opt_ = OPT_S;
            } else {
                throw std::invalid_argument("setobs: invalid 'structure' specification");
            }
        } else {
            throw std::invalid_argument("setobs: invalid key '" + akey + "'");
        }
    }

    const int argc = rcount + pcount;

    if (argc == 0 && opt_ == OPT_NONE) {
        print_smpl(dset, 0, OPT_NONE, GP_.prn_ptr());
        GP_.print();
        return;
    }

    if (argc < 2) throw std::invalid_argument("too few arguments for 'setobs'");
    if (pcount > 0) {
        if (rcount > 0 || opt_ != OPT_NONE) throw std::invalid_argument("invalid mixed modes in 'setobs'");
        // equiv. to --panel-vars
        opt_ = OPT_P;
    }

    err_ = set_obs(arg1.data(), arg2.data(), dset, opt_);
    if (err_) handle_gretl_error(err_);
}*/

void GretlDataset::setobs (const std::string &periodicity,
                           const std::string &startobs, const py::kwargs &kwargs)
{
    //
    // A method to mimic gretl's 'setobs' command
    //
    std::nullptr_t fake_dataset = nullptr;

    // we unpack kwargs;
    opt_ = unpack_kwargs_command(SETOBS, fake_dataset, kwargs);

    // we do the job (based on interact.c)
    err_ = set_obs(periodicity.c_str(), startobs.c_str(), dset, opt_);
    if (err_) handle_gretl_error(err_);
    if (dset->n > 0) {
        if (!(opt_ & (OPT_I | OPT_G))) print_smpl(dset, 0, OPT_NONE, printer_.prn_ptr());
    } else {
        pprintf(printer_.prn_ptr(), _("data frequency = %d\n"), dset->pd);
    }
    printer_.print();
}

void GretlDataset::set_as_default (void)
{
    if (auto current_default = default_dataset_.lock()) current_default->is_default = false;
    default_dataset_ = weak_from_this();
    is_default = true;
}

int GretlDataset::get_id (void) const noexcept
{
    return id_;
}

void GretlDataset::gretl_series_set_label (const int number, char *label)
{
    series_set_label(dset, number, label);
}

template <typename T>
void GretlDataset::lags (const T &varlist, int order)
{
    //
    // interface to libgretl's list_laggenr
    //
    gretl_list_ptr var_list{nullptr};

    // we construct list of external variables
    if constexpr (is_gretllist_arg_v<T>) {
        var_list = gretllist(varlist);
    } else {
        static_assert(always_false<T>, "GretlDataset::lags(): unsupported type");
    }

    // we do the job
    int *var_list_raw_ptr = var_list.release();
    err_ = list_laggenr(&var_list_raw_ptr, 1, order, nullptr, dset, 0, OPT_NONE);
    var_list.reset(var_list_raw_ptr);
    if (err_) handle_gretl_error(err_);

    // based on interact.c
    maybe_list_series(dset, printer_.prn_ptr());
    set_dataset_is_changed(dset, 1);

    printer_.print();
}
template void GretlDataset::lags<std::string>(const std::string&, int);
template void GretlDataset::lags<py::list>(const py::list&, int);

py::list GretlDataset::varnames (void) const
{
    return c_style_char_array_to_container<py::list>(dset->varname, dset->v);
}

void GretlDataset::linked_models_add (const int model_id)
{
    linked_models_.insert(model_id);
}

void GretlDataset::linked_models_del (const int model_id)
{
    linked_models_.erase(model_id);
}

std::unordered_set<int> GretlDataset::linked_models_list (void) const
{
    return linked_models_;
}

py::object GretlDataset::get_accessor (const std::string &name)
{
    return do_get_accessor<GRETL_OBJ_DSET>(name, shared_from_this(), nullptr);
}

void GretlDataset::test (const std::string &type, const std::string &varlist, const py::kwargs &kwargs)
{
    //
    // this method performs test which do take a variables list
    //
    py::dict res;
    int order = 0;
    gretl_list_ptr ex_var{nullptr};

    // we construct list of external variables
    ex_var = gretllist(varlist);

    // we set up options
    unpack_kwargs_test(opt_, type, kwargs, order);

    // we perform the test
    if (type == "adf") {
        err_ = adf_test(order, ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else if (type == "bds") {
        err_ = bds_test_driver(order, ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else if (type == "coint") {
        err_ = engle_granger_test(order, ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else if (type == "corrgm") {
        err_ = corrgram(ex_var.get()[1], order, 0, dset, opt_, printer_.prn_ptr());
    } else if (type == "difftest") {
        err_ = diff_test(ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else if (type == "fractint") {
        err_ = fractint(ex_var.get()[1], order, dset, opt_, printer_.prn_ptr());
    } else if (type == "johansen") {
        err_ = johansen_test_simple(order, ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else if (type == "kpss") {
        err_ = kpss_test(order, ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else if (type == "levinlin") {
        // if we have 'order_m' it means that values were passed as matrix
        if (kwargs.contains("order_m")) {
            // at this point it must be a py::array
            int order_m_size = kwargs["order_m"].cast<py::array>().size();
            double val;
            std::ostringstream mat;
            int i = 1;

            mat << "{";
            for (auto &item : kwargs["order_m"]) {
                val = py::cast<double>(item);
                mat << val;
                if (i == order_m_size) {
                    mat << "}";
                } else {
                    mat << ",";
                    i++;
                }
            }
            err_ = llc_test_driver(mat.str().c_str(), ex_var.get(), dset, opt_, printer_.prn_ptr());
        } else {
            err_ = llc_test_driver(std::to_string(order).c_str(), ex_var.get(), dset, opt_, printer_.prn_ptr());
        }
    } else if (type == "meantest") {
        err_ = means_test(ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else if (type == "normtest") {
        if (ex_var.get()[0] > 1) throw std::invalid_argument("normtest takes just one series");
        err_ = gretl_normality_test(ex_var.get()[1], dset, opt_, printer_.prn_ptr());
    } else if (type == "runs") {
        if (ex_var.get()[0] > 1) throw std::invalid_argument("runs takes just one series");
        err_ = runs_test(ex_var.get()[1], dset, opt_, printer_.prn_ptr());
    } else if (type == "vartest") {
        if (ex_var.get()[0] != 2) throw std::invalid_argument("vartest takes exactly two series");
        err_ = vars_test(ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else {
        throw std::invalid_argument(type + " is a model-based test");
    }
    if (err_) handle_gretl_error(err_);

    res[py::cast("test")] = get_accessor("$test");
    res[py::cast("pvalue")] = get_accessor("$pvalue");
    res[py::cast("summary")] = printer_.get_buffer();

    if (kwargs.contains("silent")) {
        auto val = kwargs["silent"];

        if (!is_bool_kwarg(val, "silent")) printer_.print();
    }

    // we register result
    move_to_python_container<GretlDataset, py::dict, py::list>(*this, std::move(res), "tests");
}

py::object GretlDataset::get_series (const std::string &series) const
{
    //
    // we return series as NumPy array
    //
    double *x = dset->Z[get_series_id(series)];
    auto res = convert_GretlType_to_GretlTypes(GRETL_TYPE_SERIES, x, dset->n, err_);

    if (err_) handle_gretl_error(err_);

    return variant_to_pyobject(std::move(*res));
}

gretl_list_ptr GretlDataset::gretllist (const std::string &formula, int *err)
{
    //
    // build a libgretl list from a model formula; if 'err' is nullptr,
    // any libgretl error is handled by handle_gretl_error(); otherwise,
    // the error code is returned to the caller
    //
    gretl_list_ptr res;
    std::string tmp = formula;
    if (err) *err = 0;

    // convert a simple R-style formula to a gretl list specification.
    std::replace(tmp.begin(), tmp.end(), '~', ' ');
    std::replace(tmp.begin(), tmp.end(), '+', ' ');

    // do the job
    res.reset(generate_list(tmp.c_str(), dset, 0, &err_));
    if (err_) {
        if (err) {
            *err = err_;
        } else {
            handle_gretl_error(err_);
        }
    }

    return res;
}

gretl_list_ptr GretlDataset::gretllist (const py::list &list, int *err)
{
    //
    // build a libgretl list from a Python list; if 'err' is nullptr,
    // any libgretl error is handled by handle_gretl_error(); otherwise,
    // the error code is returned to the caller.
    //
    gretl_list_ptr res;
    std::string formula;
    if (err) *err = 0;

    // for non-empty list, we manufacture a string to be used by generate_list()
    if (py::len(list) > 0) {
        auto vars_names = varnames();

        for (const auto &el : list) {
            if (is_pystr(el)) {
                formula += el.cast<std::string>();
            } else if (is_pyint(el)) {
                formula += vars_names[el].cast<std::string>();
            } else {
                throw std::invalid_argument("list elements must be integers or strings");
            }
            formula += " ";
        }
        gretl4py_trim(formula);
    }

    // do the job
    res.reset(generate_list(formula.c_str(), dset, 0, &err_));
    if (err_) {
        if (err) {
            *err = err_;
        } else {
            handle_gretl_error(err_);
        }
    }

    return res;
}

DNumPyArrayFC GretlDataset::hpfilt (const std::string &y, double hp_lambda, bool trend, bool one_sided)
{
    //
    // here we perform HP filtering
    // Note: we use hp_lambda instead of just lambda due to names clash on Python side
    //
    DNumPyArrayFC res(dset->n);
    gretlopt opt = trend ? OPT_T : OPT_NONE;

    // we do the job
    if (one_sided) {
        err_ = oshp_filter(dset->Z[get_series_id(y)], res.mutable_data(), dset, hp_lambda, opt);
    } else {
        err_ = hp_filter(dset->Z[get_series_id(y)], res.mutable_data(), dset, hp_lambda, opt);
    }
    if (err_) handle_gretl_error(err_);

    return res;
}

DNumPyArrayFC GretlDataset::bwfilt (const std::string &y, int n, double omega)
{
    //
    // here we perform Butterworth filtering
    //
    DNumPyArrayFC res(dset->n);

    // we do the job
    err_ = butterworth_filter(dset->Z[get_series_id(y)], res.mutable_data(), dset, n, omega);
    if (err_) handle_gretl_error(err_);

    return res;
}

DNumPyArrayFC GretlDataset::bkfilt (const std::string &y, int f1, int f2, int k)
{
    //
    // here we perform Baxter-King filtering
    //
    DNumPyArrayFC res(dset->n);

    // we do the job
    err_ = bkbp_filter(dset->Z[get_series_id(y)], res.mutable_data(), dset, f1, f2, k);
    if (err_) handle_gretl_error(err_);

    return res;
}

//
// GretlBundle class methods
//
GretlBundle::GretlBundle (void) : b_(gretl_bundle_new())
{
    if (!b_) handle_gretl_error(E_ALLOC);
}

GretlBundle::GretlBundle (MODEL *model, DATASET *dset)
{
    b_ = bundle_from_model(model, dset, &err_);
    if (err_) handle_gretl_error(err_);
}

GretlBundle::GretlBundle (GRETL_VAR *model, DATASET *dset) : b_(gretl_bundle_new())
{
    if (!b_) handle_gretl_error(E_ALLOC);
    err_ = gretl_VAR_bundlize(model, dset, b_);
    if (err_) handle_gretl_error(err_);
}

GretlBundle::GretlBundle (equation_system *model) : b_(gretl_bundle_new())
{
    if (!b_) handle_gretl_error(E_ALLOC);
    err_ = equation_system_bundlize(model, b_);
    if (err_) handle_gretl_error(err_);
}

GretlBundle::GretlBundle (gretl_bundle *src, bool copy)
{
    //
    // Here we either steal or make a copy of src
    // Note: if copy, 'src' _must_ be deallocated by libgretl
    //
    if (copy) {
        b_ = gretl_bundle_copy(src, &err_);
        free_b_ = false;
    } else {
        b_ = src;
    }
    if (err_) handle_gretl_error(err_);
}

GretlBundle::GretlBundle (const py::kwargs &kwargs) : b_(gretl_bundle_new())
{
    //
    // this is for mimics libgretl's _() bundle initializer
    //
    if (!b_) handle_gretl_error(E_ALLOC);

    // we iterate over kwargs
    for (const auto &[key, val] : kwargs) {
        auto key_str = key.cast<std::string>();

        if (is_pybool(val)) {
            set_element<bool>(key_str, val.cast<bool>());
        } else if (is_pyint(val)) {
            set_element<int>(key_str, val.cast<int>());
        } else if (is_pyfloat(val)) {
            set_element<double>(key_str, val.cast<double>());
        } else if (is_pystr(val)) {
            set_element<std::string>(key_str, val.cast<std::string>());
        } else if (is_pynumarray(val)) {
            set_element<py::array>(key_str, val.cast<py::array>());
        } else if (py::isinstance<GretlBundle>(val)) {
            set_element<GretlBundlePtr>(key_str, val.cast<GretlBundlePtr>());
        } else {
            throw std::invalid_argument("'" + key_str + "': unsupported type");
        }
    }
}

GretlBundle::~GretlBundle (void)
{
    if (free_b_ && b_ != nullptr) {
        gretl_bundle_destroy(b_);
        b_ = nullptr;
    }
}

// py::object will be registered as an attribute on python side
// py::dict will hold a content under the key
template <typename T>
void GretlBundle::extract_ (const T &t)
{
    //
    // extract the bundle and either register on python side
    // or store in as_dict
    // early exit if bundle is empty
    if (!gretl_bundle_has_content(b_)) return;

    int n_keys = 0;
    char **raw_ptr = gretl_bundle_get_keys_raw(b_, &n_keys);

    if (!raw_ptr) throw std::runtime_error("failed to obtain bundle keys");
    if (n_keys == 0) return;

    strings_array_ptr keys(raw_ptr, StringsArrayDeleter{});

    keys.get_deleter().nstrs = n_keys;
    for (int i=0; i<n_keys; ++i) {
        const char *key_cstr = keys.get()[i];
        py::str key = py::cast(key_cstr);
        py::object val = get_element<py::object>(key_cstr);

        if constexpr (std::is_same_v<T, py::object>) {
            py::setattr(t, key_cstr, val);
        } else if constexpr (std::is_same_v<T, py::dict>) {
            if (py::isinstance<GretlBundle>(val)) {
                as_dict_[key] = val.cast<GretlBundle>().to_dict();
            } else {
                as_dict_[key] = val;
            }
        } else {
            static_assert(always_false<T>, "GretlBundle::extract_(): unknown type");
        }
    }
}

int GretlBundle::get_err (void) const
{
    return err_;
}

py::dict GretlBundle::to_dict (void)
{
    extract_<py::dict>(py::dict());

    return as_dict_;
}

void GretlBundle::python_register (py::object &obj)
{
    extract_<py::object>(obj);
}

template <typename T>
T GretlBundle::print (bool tree)
{
    //
    // dedicated printer:
    // note: if 'tree', we print bundle tree representation
    //
    GretlPrint printer = GretlPrint();

    // we do the job
    if (tree) {
        err_ = gretl_bundle_print_tree(b_, printer.prn_ptr());
    } else {
        err_ = gretl_bundle_print(b_, printer.prn_ptr());
    }
    if (err_) handle_gretl_error(err_);

    // we either just print or return buffer
    if constexpr (std::is_same_v<T, void>) {
        printer.print();
    } else if constexpr (std::is_same_v<T, std::string>) {
        return printer.get_buffer();
    } else {
        static_assert(always_false<T>, "GretlBundle::print(): unknown type");
    }
}
template void GretlBundle::print<void>(bool);

std::string GretlBundle::repr_impl (void)
{
    if (err_) return "";

    return print<std::string>();
}

gretl_bundle *GretlBundle::get_ptr (void) const
{
    return b_;
}

py::list GretlBundle::keys (void)
{
    //
    // we return bundle keys as a Python list
    //
    // early exit if bundle is empty
    if (!gretl_bundle_has_content(b_)) return py::list{};

    int n_keys = 0;
    char **raw_ptr = gretl_bundle_get_keys_raw(b_, &n_keys);

    if (!raw_ptr) throw std::runtime_error("failed to obtain bundle keys");
    strings_array_ptr keys(raw_ptr, StringsArrayDeleter{});

    keys.get_deleter().nstrs = n_keys;
    if (n_keys == 0) return py::list{};

    return c_style_char_array_to_container<py::list>(keys.get(), n_keys);
}

template <typename T>
T GretlBundle::get_element (const std::string &key)
{
    //
    // we get bundle element by name (key)
    // and return either py::object or GretlTypesPtr
    //
    GretlType gtype = gretl_bundle_get_member_type(b_, key.c_str(), &err_);

    if (err_) handle_gretl_error(err_);
    auto *data = gretl_bundle_get_target(b_, key.c_str(), &gtype, &err_);

    if (err_) handle_gretl_error(err_);
    auto val = convert_GretlType_to_GretlTypes(gtype,
                                               gtype == GRETL_TYPE_SERIES ? static_cast<gretl_matrix*>(data)->val : data,
                                               gtype == GRETL_TYPE_SERIES ? static_cast<gretl_matrix*>(data)->rows : 0,
                                               err_);

    if (err_) handle_gretl_error(err_);
    if constexpr (std::is_same_v<T, py::object>) {
        return variant_to_pyobject(std::move(*val));
    } else if constexpr (std::is_same_v<T, GretlTypesPtr>) {
        return val;
    } else {
        static_assert(always_false<T>, "GretlBundle::get_element: unknown type");
    }
}

template <typename T>
void GretlBundle::set_element (const std::string &key, const T &t)
{
    //
    // we set bundle element by name (key)
    //
    using U = std::decay_t<T>;

    if constexpr (std::is_same_v<U, bool> || std::is_same_v<U, int>) {
        err_ = gretl_bundle_set_int(b_, key.c_str(), t);
    } else if constexpr (std::is_same_v<U, double>) {
        err_ = gretl_bundle_set_scalar(b_, key.c_str(), t);
    } else if constexpr (std::is_same_v<U, std::string>) {
        err_ = gretl_bundle_set_string(b_, key.c_str(), t.c_str());
    } else if constexpr (std::is_same_v<U, GretlBundlePtr>) {
        err_ = gretl_bundle_set_data(b_, key.c_str(), t->get_ptr(), GRETL_TYPE_BUNDLE);
    } else if constexpr (std::is_same_v<U, GretlMatrixPtr>) {
        err_ = gretl_bundle_set_data(b_, key.c_str(), t->get_ptr(), GRETL_TYPE_MATRIX);
    } else if constexpr (std::is_same_v<U, py::array>) {
        NumPyArrayView arr_view;
        gretl_matrix_ptr mat{nullptr};

        NumPyArray_extract(t, arr_view);
        if (!arr_view.data) throw std::runtime_error("NumPy array: null pointer.");
        mat.reset(gretl_matrix_alloc(arr_view.rows, arr_view.cols));

        NumPyArray_to_gretl_storage<gretl_matrix>(arr_view, mat.get());
        err_ = gretl_bundle_donate_data(b_, key.c_str(), mat.release(), GRETL_TYPE_MATRIX);
    } else {
        static_assert(always_false<U>, "GretlBundle::set_element(): unknown type");
    }
    if (err_) handle_gretl_error(err_);
}
template void GretlBundle::set_element<bool>(const std::string&, const bool&);
template void GretlBundle::set_element<int>(const std::string&, const int&);
template void GretlBundle::set_element<double>(const std::string&, const double&);
template void GretlBundle::set_element<std::string>(const std::string&, const std::string&);
template void GretlBundle::set_element<py::array>(const std::string&, const py::array&);
template void GretlBundle::set_element<GretlBundlePtr>(const std::string&, const GretlBundlePtr&);
template void GretlBundle::set_element<GretlMatrixPtr>(const std::string&, const GretlMatrixPtr&);

void GretlBundle::delete_element (const std::string &key)
{
    //
    // deletes the data item under @key if such an item is present
    //
    err_ = gretl_bundle_delete_data(b_, key.c_str());
    if (err_) handle_gretl_error(err_);
}

py::object GretlBundle::pop (const std::string &key)
{
    //
    // method which mimics python's pop(), i.e.
    // retrieves and deletes element under @key
    //
    py::object res = get_element<py::object>(key);
    delete_element(key);

    return res;
}

//
// GretlMatrix class methods
//
GretlMatrix::GretlMatrix (void) : m_(gretl_null_matrix_new())
{
    //
    // default constructor
    //
    if (!m_) handle_gretl_error(E_ALLOC);
}

GretlMatrix::GretlMatrix (const GretlMatrix &src): m_(gretl_matrix_copy(src.get_ptr()))
{
    //
    // copy constructor
    //
    if (!m_) handle_gretl_error(E_ALLOC);
}

GretlMatrix::GretlMatrix (int rows, int cols, double fill, const std::string &type)
{
    //
    // we allocate matrix which may be filled with zeros or ones
    //
    enum GretlMatrixType { Real, Complex };
    GretlMatrixType dtype;
    const auto ltype = gretl4py_to_lower(type);

    // basic checks
    if (ltype == "real") {
        dtype = Real;
    } else if (ltype == "complex") {
        dtype = Complex;
    } else {
        throw std::invalid_argument("matrix type must be 'real' or 'complex'");
    }

    // matrix allocation
    if (dtype == Real) {
        m_ = (fill == 0.0) ? gretl_zero_matrix_new(rows, cols) : gretl_unit_matrix_new(rows, cols);
    } else { // for Complex
        m_ = (fill == 0.0) ? gretl_cmatrix_new0(rows, cols) : gretl_cmatrix_new1(rows, cols);
    }
    if (!m_) handle_gretl_error(E_ALLOC);
    if (fill && fill != 1.0) gretl_matrix_fill(m_, fill);
}

GretlMatrix::GretlMatrix (int rows, int cols, const std::string &dist)
{
    //
    // creates a new matrix and fills it with pseudo-random
    // values from either the uniform or the standard normal distribution
    //
    auto dist_code = dist_code_from_string(dist.substr(0, 1).c_str());

    if (dist_code != D_UNIFORM && dist_code != D_NORMAL) throw std::invalid_argument("unknown distribution code: '" + dist + "'");
    m_ = gretl_random_matrix_new(rows, cols, dist_code);
    if (!m_) handle_gretl_error(E_ALLOC);
}

GretlMatrix::GretlMatrix (double start, double end, double step)
{
    //
    // row vector, containing values from @start to @end, in decreasing order if @start > @end
    //
    m_ = gretl_matrix_seq(start, end, step, &err_);
    if (err_) handle_gretl_error(err_);
}

GretlMatrix::GretlMatrix (int n) : m_(gretl_identity_matrix_new(n))
{
    //
    // identity matrix
    //
    if (!m_) handle_gretl_error(E_ALLOC);
}

GretlMatrix::GretlMatrix (const py::array &array)
{
    //
    // from NumPy source
    //
    m_ = numpy_to_matrix_(array);
}

GretlMatrix::GretlMatrix (gretl_matrix *src, bool copy)
{
    //
    // copy == true means deep copy, false == we "steal" pointer
    //
    copy ? m_ = gretl_matrix_copy(src) : m_ = src;
    if (!m_) handle_gretl_error(E_ALLOC);
}

GretlMatrix::GretlMatrix (const py::list &list)
{
    //
    // we convert Python list to GretlMatrix
    //
    // for empty list, we return empty matrix
    if (!list.size()) {
        m_ = gretl_null_matrix_new();
        if (!m_) handle_gretl_error(E_ALLOC);
        return;
    }

    // do the job
    py::module_ np = py::module_::import("numpy");
    py::object array_obj = np.attr("asarray")(list, py::arg("dtype") = py::none());
    py::array array = py::reinterpret_borrow<py::array>(array_obj);

    m_ = numpy_to_matrix_(array);
}

GretlMatrix::~GretlMatrix (void)
{
    if (m_) {
        gretl_matrix_free(m_);
        m_ = nullptr;
    }
}

//
// private methods
//
// static
gretl_matrix *GretlMatrix::numpy_to_matrix_ (const py::array& array)
{
    //
    // from NumPy source
    // note: the allocated memory will be owned by caller
    //
    NumPyArrayView arr_view;
    gretl_matrix *res = nullptr;

    NumPyArray_extract(array, arr_view);
    if (!arr_view.data) throw std::runtime_error("NumPy array: null pointer.");
    if (arr_view.is_complex) {
        res = gretl_cmatrix_new(arr_view.rows, arr_view.cols);
    } else {
        res = gretl_matrix_alloc(arr_view.rows, arr_view.cols);
    }
    if (!res) handle_gretl_error(E_ALLOC);
    NumPyArray_to_gretl_storage<gretl_matrix>(arr_view, res);

    return res;
}

// instance
void GretlMatrix::check_bounds_ (int i, int j) const
{
    if (i < 0 || j < 0 || i >= m_->rows || j >= m_->cols)
        handle_gretl_error(E_BOUNDS, "[i,j]", "[" + std::to_string(i) + "," + std::to_string(j) + "]");
}

gretl_matrix_ptr GretlMatrix::exec_slice_ (const std::string &slice_str) const
{
    //
    // here we execute libgretl apparatus to get sliced matrix
    //
    const std::string tmp = "m_gretlpy__";
    std::string expr = tmp + "[" + slice_str + "]";
    gretl_matrix_ptr res{nullptr};
    int err = 0;

    err_ = private_matrix_add_as_shell(m_, tmp.c_str());
    if (err_) handle_gretl_error(err_);
    res.reset(generate_matrix(expr.c_str(), nullptr, &err));
    err_ = user_var_delete_by_name(tmp.c_str(), nullptr);
    if (err) handle_gretl_error(err);
    if (err_) handle_gretl_error(err_);

    return res;
}

std::string GretlMatrix::render_slice_ (const py::tuple &slice) const
{
    std::string res;

    for (auto item : slice) {
        std::string tmp;

        if (is_pyslice(item)) {
            tmp = python_slice_to_gretl_slice(item.cast<py::slice>()).slice_str;
        } else if (is_pyint(item)) {
            py::print("wchodzę do int-ów");
            tmp = gretl_slice_from_int(item.cast<int>());
        } else {
            throw std::invalid_argument("bad slicer passed");
        }
        if (!res.empty()) res += ",";
        res += (tmp == ":" ? " " : tmp);
    }

    return gretl4py_trim(res);
}

std::string GretlMatrix::render_slice_ (const py::slice &slice) const
{
    return python_slice_to_gretl_slice(slice).slice_str;
}

std::string GretlMatrix::render_slice_ (const std::string &slice) const
{
    return python_slice_to_gretl_slice(slice).slice_str;
}

//
// public methods
//
gretl_matrix *GretlMatrix::get_ptr (void) const
{
    return m_;
}

double *GretlMatrix::val (void) const
{
    return m_->val;
}

double _Complex *GretlMatrix::z (void) const
{
    if (!is_complex()) throw std::runtime_error("not a complex matrix");

    return m_->z;
}

int GretlMatrix::rows (void) const
{
    return m_->rows;
}

int GretlMatrix::cols (void) const
{
    return m_->cols;
}

[[nodiscard]] bool GretlMatrix::is_complex (void) const noexcept
{
    return m_->is_complex != 0;
}

std::string GretlMatrix::repr_impl (void)
{
    err_ = 0;

    if (is_complex()) {
        err_ = gretl_cmatrix_print(m_, nullptr, printer_.prn_ptr());
    } else {
        gretl_matrix_print_to_prn(m_, nullptr, printer_.prn_ptr());
    }
    if (err_) handle_gretl_error(err_);

    return printer_.get_buffer();
}

// ROWS == true: we deal we rows of the matrix
template <bool ROWS>
py::list GretlMatrix::rows_cols_names_get (void) const
{
    //
    // method which retrieves rows/cols names
    //
    const char **orig_names;
    int n = 0;

    if (is_complex()) throw std::runtime_error("complex arguments/operands not supported");

    if constexpr (ROWS) {
        orig_names = gretl_matrix_get_rownames(m_);
    } else {
        orig_names = gretl_matrix_get_colnames(m_);
    }
    if (!orig_names) return py::list{};

    if constexpr (ROWS) {
        n = m_->rows;
    } else {
        n = m_->cols;
    }

    return c_style_char_array_to_container<py::list>(orig_names, n);
}
template py::list GretlMatrix::rows_cols_names_get<true>(void) const;
template py::list GretlMatrix::rows_cols_names_get<false>(void) const;

template<bool ROWS>
void GretlMatrix::rows_cols_names_set (const py::list &src) const
{
    //
    // we set rows/cols based on src
    // note: empty src means we erase rows/cols names
    //
    strings_array_ptr names(nullptr, StringsArrayDeleter{});
    int n;

    if (is_complex()) throw std::runtime_error("complex arguments/operands not supported");

    // for empty list, we erase rows/cols names, otherwise we record the length
    if constexpr (ROWS) {
        if (src.empty()) {
            gretl_matrix_set_rownames(m_, nullptr);
            return;
        } else {
            n = m_->rows;
        }
    } else {
        if (src.empty()) {
            gretl_matrix_set_colnames(m_, nullptr);
            return;
        } else {
            n = m_->cols;
        }
    }

    // we allocate char** storage (unique_ptr)
    if (src.size() != static_cast<size_t>(n)) handle_gretl_error(E_INVARG, "names", "supplied list should be of length: " + std::to_string(n));
    char **dst = strings_array_new_with_length(n, OBSLEN);
    if (dst == nullptr) handle_gretl_error(E_ALLOC);
    names.get_deleter().nstrs = n;
    names.reset(dst);

    // we do the job
    for (int i=0; i<n; ++i) {
        std::string s = py::cast<std::string>(src[i]);

        std::strncpy(names.get()[i], s.c_str(), OBSLEN-1);
        names.get()[i][OBSLEN-1] = '\0';
    }
    if constexpr (ROWS) {
        gretl_matrix_set_rownames(m_, names.release());
    } else {
        gretl_matrix_set_colnames(m_, names.release());
    }
}
template void GretlMatrix::rows_cols_names_set<true>(const py::list&) const;
template void GretlMatrix::rows_cols_names_set<false>(const py::list&) const;

py::object GretlMatrix::get_element (int i, int j) const
{
    //
    // method for getting matrix element [i, j]
    //
    if (i < 0 || j < 0) {
        check_bounds_(abs(i), abs(j));
        auto res = std::make_unique<GretlTypes>();
        auto mat = exec_slice_(gretl_slice_from_int(i) + "," + gretl_slice_from_int(j));

        *res = std::make_shared<GretlMatrix>(mat.release(), false);

        return variant_to_pyobject(std::move(*res));
    }
    check_bounds_(i, j);
    if (is_complex()) {
        return py::cast(c_complex_to_std_complex(gretl_cmatrix_get(m_, i, j)));
    } else {
        return py::cast(gretl_matrix_get(m_, i, j));
    }
}

GretlMatrixPtr GretlMatrix::get_slice (const py::tuple &slice) const
{
    auto res = exec_slice_(render_slice_(slice));

    return std::make_shared<GretlMatrix>(res.release());
}

GretlMatrixPtr GretlMatrix::get_slice (const py::slice &slice) const
{
    std::string slice_str;
    py::object start = slice.attr("start");
    py::object stop = slice.attr("stop");

    if (is_pynone(start) && is_pynone(stop)) {
        slice_str = ",";
    } /*else if (is_pynone(start) && is_pyint(stop)) {
        slice_str = "," + gretl_slice_from_int(stop.cast<int>());
    } else if (is_pyint(start) && is_pynone(stop)) {
        slice_str = gretl_slice_from_int(start.cast<int>()) + ",";
    } else if (is_pyint(start) && is_pyint(stop)) {
        slice_str = gretl_slice_from_int(start.cast<int>()) + "," + gretl_slice_from_int(stop.cast<int>());
    }*/
    else if ((is_pynone(start) || is_pynone(stop)) && (std::min)(m_->rows, m_->cols) > 1) {
        auto cols_slice = py::slice(py::none(), py::none(), py::none());
        py::tuple tuple = py::make_tuple(slice, cols_slice);
        py::print("jedno jest none");

        slice_str = render_slice_(tuple);
        py::print("wyprodukowany string:", slice_str);
    } else {
        slice_str = render_slice_(slice);
    }
    py::print("slice_str (get_slice)", slice_str);
    auto res = exec_slice_(slice_str);

    return std::make_shared<GretlMatrix>(res.release());
}

GretlMatrixPtr GretlMatrix::get_slice (const std::string &slice) const
{
    auto res = exec_slice_(render_slice_(slice));

    return std::make_shared<GretlMatrix>(res.release());
}

//T: double, std::complex<double>
template <typename T>
void GretlMatrix::set_element (int i, int j, T val)
{
    //
    // method for setting 'val' as matrix element [i, j]
    //
    check_bounds_(i, j);

    if constexpr (std::is_same_v<T, double>) {
        if (is_complex()) throw std::invalid_argument("matrix is complex, assign a complex number (e.g. 1+2j)");
        gretl_matrix_set(m_, i, j, val);
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        if (!is_complex()) throw std::invalid_argument("matrix is real, use double");
        double _Complex z = { val.real(), val.imag() };

        gretl_cmatrix_set(m_, i, j, z);
    } else {
        static_assert(always_false<T>, "GretlMatrix::set_element(): unknown type");
    }
}
template void GretlMatrix::set_element<double>(int i, int j, double val);
template void GretlMatrix::set_element<std::complex<double>>(int i, int j, std::complex<double> val);

py::object GretlMatrix::to_numpy() const
{
    //
    // here we construct NumPy as a deep copy of gretl_matrix
    //
    auto res = std::make_unique<GretlTypes>();

    // for complex type
    if (is_complex()) {
        DNumPyArrayZ mat({m_->rows, m_->cols});

        std::memcpy(reinterpret_cast<std::byte*>(mat.mutable_data()),
                    reinterpret_cast<const std::byte*>(m_->z),
                    m_->rows * m_->cols * sizeof(std::complex<double>));
        if ((std::min)(m_->rows, m_->cols) == 1) mat = mat.reshape({mat.size()});
        *res = mat;
    } else {
        DNumPyArrayFC mat({m_->rows, m_->cols});

        std::memcpy(mat.mutable_data(), m_->val, m_->rows * m_->cols * sizeof(double));
        if ((std::min)(m_->rows, m_->cols) == 1) mat = mat.reshape({mat.size()});
        *res = mat;
    }

    return variant_to_pyobject(std::move(*res));
}

GretlMatrixPtr GretlMatrix::neg (void) const
{
    //
    // we return neqation, i.e. (-1 * m_)
    //
    if (is_complex()) return alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::MUL>(-1);

    return alg_dot_operations<double, GretlMatrixAlgOperations::MUL>(-1);
}

// T: GretlMatrix, py::array
// KIND: ADD, SUB, MATMUL, TRUEDIV
template <typename T, GretlMatrixAlgOperations KIND>
GretlMatrixPtr GretlMatrix::alg_operations (const T &rhs) const
{
    //
    // matrices standard operations
    // used as a backend for: __add__, __sub__, __matmul__, __truediv__
    //
    gretl_matrix *rhs_matrix = nullptr;         // right-hand side, used in any kind of operation
    gretl_matrix_ptr rhs_storage{nullptr};      // if we need a temporary unique_ptr to rhs (for py::array)
    gretl_matrix_ptr res{nullptr};

    // we check template parameters
    if constexpr (std::is_same_v<T, GretlMatrix>) {
        rhs_matrix = rhs.get_ptr();
    } else if constexpr (std::is_same_v<T, py::array>) {
        rhs_storage.reset(numpy_to_matrix_(rhs));
        rhs_matrix = rhs_storage.get();
    } else {
        static_assert(always_false<T>, "GretlMatrix::alg_mat_mat_operations(): unknown type");
    }

    // we do the job
    if constexpr (KIND == GretlMatrixAlgOperations::ADD) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_add_sub(m_, rhs_matrix, 1, &err_));
        } else {
            res.reset(gretl_matrix_alloc(m_->rows, m_->cols));
            err_ = gretl_matrix_add(m_, rhs_matrix, res.get());
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::SUB) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_add_sub(m_, rhs_matrix, -1, &err_));
        } else {
            res.reset(gretl_matrix_alloc(m_->rows, m_->cols));
            err_ = gretl_matrix_subtract(m_, rhs_matrix, res.get());
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::MATMUL) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_multiply(m_, rhs_matrix, &err_));
        } else {
            res.reset(gretl_matrix_multiply_new(m_, rhs_matrix, &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::TRUEDIV) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_divide(m_, rhs_matrix, GRETL_MOD_TRANSPOSE, &err_));
        } else {
            res.reset(gretl_matrix_divide(m_, rhs_matrix, GRETL_MOD_TRANSPOSE, &err_));
        }
    } else {
        static_assert(always_false<T>, "Unsupported GretlMatrixAlgOperations value");
    }
    if (err_) handle_gretl_error(err_);

    return std::make_shared<GretlMatrix>(res.release(), false);
}
template GretlMatrixPtr GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::ADD>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::ADD>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::SUB>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::SUB>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::MATMUL>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::MATMUL>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::TRUEDIV>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::TRUEDIV>(const py::array &rhs) const;

// T: GretlMatrix, py::array, double
// KIND: IADD, ISUB, IMUL, ITRUEDIV, IPOW
template <typename T, GretlMatrixAlgOperations KIND>
GretlMatrix &GretlMatrix::alg_inplace_operations (const T &rhs)
{
    //
    // in-place operations
    // note: here we perform some scalar operations as well
    // used as a backend for: __iadd__, __isub__, __imul__, __itruediv__, __ipow__
    //
    gretl_matrix *rhs_matrix = nullptr;           // right-hand side, used in any kind of operation
    gretl_matrix_ptr rhs_storage{nullptr};        // if we need a temporary unique_ptr to rhs (for py::array)

    // we define scalar-only operations
    constexpr bool scalar_only = (KIND == GretlMatrixAlgOperations::IMUL
    || KIND == GretlMatrixAlgOperations::ITRUEDIV || (KIND == GretlMatrixAlgOperations::IPOW && std::is_same_v<T, double>));

    // we check template parameters
    if constexpr (std::is_same_v<T, GretlMatrix>) {
        rhs_matrix = rhs.get_ptr();
    } else if constexpr (std::is_same_v<T, py::array>) {
        rhs_storage.reset(numpy_to_matrix_(rhs));
        rhs_matrix = rhs_storage.get();
    } else if constexpr (std::is_same_v<T, double>) {
        if constexpr (!scalar_only) {
            rhs_storage.reset(gretl_matrix_alloc(1, 1));
            rhs_matrix = rhs_storage.get();
            gretl_matrix_fill(rhs_matrix, rhs);
        }
    } else {
        static_assert(always_false<T>, "GretlMatrix::alg_mat_mat_inplace_operations(): unknown type");
    }

    // we exclude complex operations
    if constexpr (!scalar_only) {
        if (is_complex() && rhs_matrix->is_complex) {
            handle_gretl_error(E_CMPLX);
        } else if (is_complex() || rhs_matrix->is_complex) {
            handle_gretl_error(E_MIXED);
        }
    }

    // we do the job
    if constexpr (KIND == GretlMatrixAlgOperations::IADD) {
        err_ = gretl_matrix_add_to(m_, rhs_matrix);
    } else if constexpr (KIND == GretlMatrixAlgOperations::ISUB) {
        err_ = gretl_matrix_subtract_from(m_, rhs_matrix);
    } else if constexpr (KIND == GretlMatrixAlgOperations::IMUL && std::is_same_v<T, double>) {
        gretl_matrix_multiply_by_scalar(m_, rhs);
    } else if constexpr (KIND == GretlMatrixAlgOperations::ITRUEDIV && std::is_same_v<T, double>) {
        gretl_matrix_divide_by_scalar(m_, rhs);
    } else if constexpr (KIND == GretlMatrixAlgOperations::IPOW && !std::is_same_v<T, double>) {
        for (int i=0; i<m_->rows*m_->cols; ++i) {
            m_->val[i] = std::pow(m_->val[i], rhs_matrix->val[i]);
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::IPOW && std::is_same_v<T, double>) {
        for (int i=0; i<m_->rows*m_->cols; ++i) {
            m_->val[i] = std::pow(m_->val[i], rhs);
        }
    } else {
        static_assert(always_false<T>, "Unsupported GretlMatrixAlgOperations value");
    }
    if (err_) handle_gretl_error(err_);

    return *this;
}
template GretlMatrix &GretlMatrix::alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::IADD>(const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<py::array, GretlMatrixAlgOperations::IADD>(const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IADD>(const double &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::ISUB>(const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<py::array, GretlMatrixAlgOperations::ISUB>(const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::ISUB>(const double &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IMUL>(const double &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::ITRUEDIV>(const double &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::IPOW>(const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<py::array, GretlMatrixAlgOperations::IPOW>(const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IPOW>(const double &rhs);

// T: GretlMatrix, py::array, std::complex<double>
// KIND: IADD, ISUB, IMUL, ITRUEDIV
template <typename T, GretlMatrixAlgOperations KIND>
GretlMatrix &GretlMatrix::alg_inplace_operations_complex (const T &rhs)
{
    //
    // here we implement non-existing in libgretl inplace operations for complex matrices
    // used as a backend for: __iadd__, __isub__, __imul__, __itruediv__, __ipow__
    //
    gretl_matrix *rhs_matrix = nullptr;           // right-hand side, used in any kind of operation
    gretl_matrix_ptr rhs_storage{nullptr};        // if we need a temporary unique_ptr to rhs (for py::array)
    double _Complex z;

    // we check template parameters
    if constexpr (std::is_same_v<T, GretlMatrix>) {
        rhs_matrix = rhs.get_ptr();
    } else if constexpr (std::is_same_v<T, py::array>) {
        rhs_storage.reset(numpy_to_matrix_(rhs));
        rhs_matrix = rhs_storage.get();
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        if constexpr (KIND != GretlMatrixAlgOperations::IPOW) {
            z = { rhs.real(), rhs.imag() };
        }
    } else {
        static_assert(always_false<T>, "GretlMatrix::alg_inplace_operations_complex(): unknown type");
    }

    // we check matrices features
    if (!is_complex()) throw std::invalid_argument("left matrix is real, should be complex");
    if constexpr (std::is_same_v<T, GretlMatrix> || std::is_same_v<T, py::array>) {
        if (!rhs_matrix->is_complex) throw std::invalid_argument("right matrix is real, should be complex");
        if (m_->rows != rhs_matrix->rows || m_->cols != rhs_matrix->cols) handle_gretl_error(E_NONCONF);
    }

    // we do the job
    for (int i=0; i<m_->rows * m_->cols; ++i) {
        if constexpr (KIND == GretlMatrixAlgOperations::IADD) {
            if constexpr (std::is_same_v<T, std::complex<double>>) {
                m_->z[i] += z;
            } else {
                m_->z[i] += rhs_matrix->z[i];
            }
        } else if constexpr (KIND == GretlMatrixAlgOperations::ISUB) {
            if constexpr (std::is_same_v<T, std::complex<double>>) {
                m_->z[i] -= z;
            } else {
                m_->z[i] -= rhs_matrix->z[i];
            }
        } else if constexpr (KIND == GretlMatrixAlgOperations::IMUL) {
            if constexpr (std::is_same_v<T, std::complex<double>>) {
                m_->z[i] *= z;
            } else {
                static_assert(always_false<T>, "GretlMatrix::alg_inplace_operations_complex(): unsupported type");
            }
        } else if constexpr (KIND == GretlMatrixAlgOperations::ITRUEDIV) {
            if constexpr (std::is_same_v<T, std::complex<double>>) {
                m_->z[i] /= z;
            } else {
                static_assert(always_false<T>, "GretlMatrix::alg_inplace_operations_complex(): unsupported type");
            }
        } else if constexpr (KIND == GretlMatrixAlgOperations::IPOW) {
            auto base = c_complex_to_std_complex(m_->z[i]);

            if constexpr (std::is_same_v<T, std::complex<double>>) {
                m_->z[i] = std_complex_to_c_complex(std::pow(base, rhs));
            } else {
                m_->z[i] = std_complex_to_c_complex(std::pow(base, c_complex_to_std_complex(rhs_matrix->z[i])));
            }
        }
    }

    return *this;
}
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::IADD> (const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::IADD> (const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IADD> (const std::complex<double> &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::ISUB> (const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::ISUB> (const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::ISUB> (const std::complex<double> &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IMUL> (const std::complex<double> &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::ITRUEDIV> (const std::complex<double> &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::IPOW> (const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::IPOW> (const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IPOW> (const std::complex<double> &rhs);

// T: GretlMatrix or py::array, double, std::complex<double>>
// KIND: ADD, SUB, MUL, TRUEDIV, POW
template <typename T, GretlMatrixAlgOperations KIND>
GretlMatrixPtr GretlMatrix::alg_dot_operations (const T &rhs) const
{
    //
    // matrices element-wise (libgretl's 'dot') operations
    // note: here we perform some scalar operations as well
    // used as a backend for: __add__, __sub__, __mul__, __truediv__, __pow__
    //
    gretl_matrix *rhs_matrix = nullptr;           // right-hand side, used in any kind of operation
    gretl_matrix_ptr rhs_storage{nullptr};        // if we need a temporary unique_ptr to rhs (for py::array)
    gretl_matrix_ptr res{nullptr};

    // we check template parameters
    if constexpr (std::is_same_v<T, GretlMatrix>) {
        rhs_matrix = rhs.get_ptr();
    } else if constexpr (std::is_same_v<T, py::array>) {
        rhs_storage.reset(numpy_to_matrix_(rhs));
        rhs_matrix = rhs_storage.get();
    } else if constexpr (std::is_same_v<T, double>) {
        rhs_storage.reset(gretl_matrix_alloc(1, 1));
        rhs_matrix = rhs_storage.get();
        gretl_matrix_set(rhs_matrix, 0, 0, rhs);
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        double _Complex z = { rhs.real(), rhs.imag() };

        rhs_storage.reset(gretl_cmatrix_new(1, 1));
        rhs_matrix = rhs_storage.get();
        gretl_cmatrix_set(rhs_matrix, 0, 0, z);
    } else {
        static_assert(always_false<T>, "GretlMatrix::alg_mat_mat_dot_operations(): unknown type");
    }

    // we do the job
    if constexpr (KIND == GretlMatrixAlgOperations::ADD) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '+', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '+', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::SUB) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '-', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '-', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::MUL) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '*', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '*', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::TRUEDIV) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '/', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '/', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::POW) {
        //(has_negative_base(m_) && has_non_integer_exponent(rhs_matrix))
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '^', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '^', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::EQ) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '=', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '=', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::NE) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '!', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '!', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::LT) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '<', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '<', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::LE) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '[', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '[', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::GT) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '>', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '>', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::GE) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, ']', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, ']', &err_));
        }
    } else {
        static_assert(always_false<T>, "Unsupported GretlMatrixAlgOperations value");
    }
    if (err_) handle_gretl_error(err_);

    return std::make_shared<GretlMatrix>(res.release(), false);
}
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::ADD>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::ADD>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::SUB>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::SUB>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::MUL>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::MUL>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::MUL>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::MUL>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::TRUEDIV>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::TRUEDIV>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::POW>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::POW>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::POW>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::POW>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::EQ>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::EQ>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::EQ>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::EQ>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::NE>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::NE>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::NE>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::NE>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::LT>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::LT>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::LT>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::LT>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::LE>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::LE>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::LE>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::LE>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::GT>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::GT>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::GT>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::GT>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::GE>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::GE>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::GE>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::GE>(const std::complex<double> &rhs) const;
