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
#include "gretl4py_dataset.h"
#include "gretl4py_model.h"
#include "gretl4py_packages.h"
#include "gretl4py_utils.h"
#include <gretl/libset.h>
#include <gretl/matrix_extra.h>
extern "C" {
    #include <gretl/var.h>
    #include <gretl/gretl_xml.h>
}

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
        el.copy(dset->S[i], OBSLEN-1, 0);
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
    if (!dset) handle_gretl_error(E_ALLOC);
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

    // we do the job
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

    // finally, we prepare summary
    if (v > 0) {
        err_ = pprintf(printer_.prn_ptr(), "Replaced series %s (ID %d)", name.c_str(), v);
    } else {
        // we setup name and description for new series
        v = dset->v-1;
        err_ = rename_series(dset, v, name.c_str(), OPT_NONE);
        if (err_) handle_gretl_error(err_);
        gretl_series_set_label(v, label.get());
        err_ = pprintf(printer_.prn_ptr(), "Generated series %s (ID %d)", name.c_str(), v);
    }
    if (err_ == -1) handle_gretl_error(E_ALLOC);
    printer_.print();
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

[[nodiscard]] bool GretlDataset::is_panel (void) const noexcept
{
    return static_cast<bool>(dataset_is_panel(dset));
}

[[nodiscard]] bool GretlDataset::is_time_series (void) const noexcept
{
    return static_cast<bool>(dataset_is_time_series(dset));
}

[[nodiscard]] bool GretlDataset::is_cross_section (void) const noexcept
{
    return static_cast<bool>(dataset_is_cross_section(dset));
}

std::string GretlDataset::summary (bool print) const noexcept
{
    //
    // method prints basic info about dataset
    //
    auto *prn = printer_.prn_ptr();

    print_sample_status(dset, prn);
    pprintf(prn, "\n");
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

template <typename T>
void GretlDataset::to_file (const std::string &filename, const T &varlist, const py::kwargs &kwargs)
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
template void GretlDataset::to_file<std::string>(const std::string&, const std::string&, const py::kwargs&);
template void GretlDataset::to_file<py::list>(const std::string&, const py::list&, const py::kwargs&);

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

char *GretlDataset::add_series (gretl_double_ptr data, const std::string &name, int n, gretl_gchar_ptr label)
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

    // we do the job (if dimensions match, we take fast track)
    if ((std::max)(view.cols, view.rows) == dset->n) {
        // we use std::malloc since memory will be deallocated by libgretl
        gretl_double_ptr data_copy(static_cast<double*>(std::malloc(dset->n * sizeof(double))));

        // actual work
        NumPyArray_to_gretl_storage<double>(view, data_copy.get(), 0, true);
        add_or_replace_allocated_series_(std::move(data_copy), name, dset->n);
    } else {
        add_series(GretlMatrix(data), name);
    }
}

void GretlDataset::add_series (const GretlMatrix &data, const std::string &name)
{
    //
    // Here we add existing GretlMatrix to dataset
    // Note: the underline data is copied (deep copy)
    //

    if ((std::min)(data.cols(), data.rows()) > 1) throw std::invalid_argument("given variable is not a vector");
    if (data.is_complex()) throw std::invalid_argument("given matrix is complex, must be real");

    // we do the job (if dimensions match, we take fast track)
    if ((std::max)(data.cols(), data.rows()) == dset->n) {
        // we use std::malloc since memory will be deallocated by libgretl
        gretl_double_ptr data_copy(static_cast<double*>(std::malloc(dset->n * sizeof(double))));

        std::memcpy(data_copy.get(), data.val(), dset->n * sizeof(double));
        add_or_replace_allocated_series_(std::move(data_copy), name, dset->n);
    } else {
        const char *tmp_matrix_name = "gretl4py_mattodset";

        genr<GretlMatrix>(tmp_matrix_name, data);
        gretl_generate<GRETL_TYPE_SERIES>(tmp_matrix_name, shared_from_this(), name);
        do_gretl_delete_variables(tmp_matrix_name, py::dict(), false);
    }
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

void GretlDataset::print (const py::list &varlist, const py::kwargs &kwargs)
{
    //
    // wrapper for varlist passed as list
    //
    std::string varlist_str = "";

    if (!varlist.empty()) {
        auto list = gretllist(varlist);

        // we list is non-empty, we produce its string reprezentation
        if (list) {
            gretl_char_ptr list_str(gretl_list_to_string(list.get(), dset, &err_));

            if (err_) handle_gretl_error(err_);
            varlist_str = std::string(list_str.get());
        }
    }

    // we do the job
    print(varlist_str, kwargs);
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
        err_ = perma_sample(dset, opt_, printer_.prn_ptr(), nullptr);                                               // --permanent, by itself
    } else if (opt_ & (OPT_U | OPT_X)) {
        err_ = set_panel_sample(startobs.c_str(), endobs.c_str(), opt_, dset, nullptr, printer_.prn_ptr());         // panel data: --unit or --time option
    } else if (smpl_restrict(opt_)) {
        err_ = restrict_sample(startobs.c_str(), var_list.get(), dset, nullptr, opt_, printer_.prn_ptr(), nullptr); // --restrict, --dummy, etc.
    } else if (startobs.empty() && endobs.empty()) {
        print_smpl(dset, get_full_length_n(), OPT_F, printer_.prn_ptr());                                           // no args given: give a report
        //print_smpl(dset, get_full_length_n(), OPT_NONE, GP_.prn_ptr());
        report = true;
    } else {
        err_ = set_sample(startobs.c_str(), endobs.c_str(), dset, opt_);                                            // simple setting of t1, t2 business /
    }
    if (err_) handle_gretl_error(err_);

    if (!report && !(opt_ & OPT_Q)) print_smpl(dset, get_full_length_n(), OPT_NONE, printer_.prn_ptr());
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

template <typename T>
void GretlDataset::do_modify_dataset (DatasetOp op, const T &list, const std::string &param, const py::kwargs &kwargs)
{
    //
    // interface to libgretl's modify_dataset(),
    // the underlying engine for the DATAMOD command
    //
    std::nullptr_t fake_dataset = nullptr;
    gretl_list_ptr var_list{nullptr};
    const char *param_ptr;

    // we construct list of external variables
    if (op == DS_SORTBY || op == DS_DSORTBY) {
        if constexpr (is_gretllist_arg_v<T>) {
            var_list = gretllist(list);
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
            ;
        } else {
            static_assert(always_false<T>, "GretlDataset::do_modify_dataset(): unsupported type");
        }
    }

    // we unpack kwargs;
    opt_ = unpack_kwargs_command(DATAMOD, fake_dataset, kwargs);

    // we handle special cases here
    if (op == DS_EXPAND && param == "-999") {
        param_ptr = nullptr;
    } else {
        param_ptr = param.c_str();
    }

    // we do the job
    err_ = modify_dataset(dset, op, var_list.get(), param_ptr, opt_, printer_.prn_ptr());
    if (err_) handle_gretl_error(err_);
    printer_.print();
}
template void GretlDataset::do_modify_dataset<std::string>(DatasetOp, const std::string&, const std::string&, const py::kwargs&);
template void GretlDataset::do_modify_dataset<py::list>(DatasetOp, const py::list&, const std::string&, const py::kwargs&);
template void GretlDataset::do_modify_dataset<std::nullptr_t>(DatasetOp, const std::nullptr_t&, const std::string&, const py::kwargs&);

GretlMatrixPtr GretlDataset::tdisagg (py::object Y, int s, const py::kwargs &kwargs)
{
    // we exclude a pure matrix-based cases
    if (is_pygretlmatrix(Y)) {
        if (!kwargs.contains("X")) {
            const std::string msg = "is gretl.Matrix object, please use gretl.tdisagg() instead";
            handle_gretl_error(E_INVARG, "Y", msg);
        } else {
            auto X = kwargs["X"];

            if (!X.is_none() && is_pygretlmatrix(X)) {
                const std::string msg = "both arguments are gretl.Matrix objects, please use gretl.tdisagg() instead";
                handle_gretl_error(E_INVARG, "Y and X", msg);
            }
        }
    }

    return do_get_tdisagg_matrix(Y, s, kwargs, shared_from_this());
}

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

GretlBundlePtr GretlDataset::test (const std::string &type, const std::string &varlist, const py::kwargs &kwargs)
{
    //
    // this method performs test which do take a variables list
    //
    GretlBundlePtr res = std::make_shared<GretlBundle>();
    gretl_list_ptr ex_var{nullptr};
    int order = 0;
    bool has_result = false;    // if given test produces additional results accessible via $result

    // we construct list of external variables
    ex_var = gretllist(varlist);

    // we set up options
    auto printout = unpack_kwargs_test(opt_, type, kwargs, order);

    // we perform the test
    if (type == "adf") {
        err_ = adf_test(order, ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else if (type == "bds") {
        err_ = bds_test_driver(order, ex_var.get(), dset, opt_, printer_.prn_ptr());
        has_result = true;
    } else if (type == "coint") {
        err_ = engle_granger_test(order, ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else if (type == "corrgm") {
        if (ex_var.get()[0] != 1) throw std::invalid_argument("corrgm takes just one series");
        err_ = corrgram(ex_var.get()[1], order, 0, dset, opt_, printer_.prn_ptr());
    } else if (type == "difftest") {
        err_ = diff_test(ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else if (type == "fractint") {
        if (ex_var.get()[0] != 1) throw std::invalid_argument("fractint takes just one series");
        err_ = fractint(ex_var.get()[1], order, dset, opt_, printer_.prn_ptr());
        has_result = true;
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
        if (ex_var.get()[0] != 1) throw std::invalid_argument("normtest takes just one series");
        err_ = gretl_normality_test(ex_var.get()[1], dset, opt_, printer_.prn_ptr());
    } else if (type == "runs") {
        if (ex_var.get()[0] != 1) throw std::invalid_argument("runs takes just one series");
        err_ = runs_test(ex_var.get()[1], dset, opt_, printer_.prn_ptr());
    } else if (type == "vartest") {
        if (ex_var.get()[0] != 2) throw std::invalid_argument("vartest takes exactly two series");
        err_ = vars_test(ex_var.get(), dset, opt_, printer_.prn_ptr());
    } else {
        throw std::invalid_argument(type + " is a model-based test");
    }
    if (err_) handle_gretl_error(err_);

    // we printout results
    if (printout) printer_.print();

    // material we return
    res->set_element<std::string>("type", type);
    res->set_element("options", py::object(kwargs));
    res->set_element("test", get_accessor("$test"));
    res->set_element("pvalue", get_accessor("$pvalue"));
    res->set_element<std::string>("summary", std::string(printer_.get_buffer()));
    if (has_result) res->set_element("result", get_accessor("$result"));

    // we register result
    move_to_python_container<GretlDataset, py::dict, py::list>(*this, res->to_dict(), "tests");

    return res;
}

GretlMatrixPtr GretlDataset::get_series (const std::string &series) const
{
    //
    // we return series as GretlMatrix array
    //
    auto vector = gretl_vector_from_series(dset->Z[get_series_id(series)], dset->t1, dset->t2);
    if (!vector) handle_gretl_error(E_ALLOC);

    return std::make_shared<GretlMatrix>(static_cast<gretl_matrix*>(vector), false);
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
    return gretllist(python_list_to_formula(list), err);
}

std::string GretlDataset::python_list_to_formula (const py::list &list) const
{
    //
    // here we construct a gretl formula from a Python list
    //
    if (py::len(list) == 0) return {};
    std::string formula;
    auto var_names = varnames();
    int list_size = list.size();

    for (int i=0; i<list_size; ++i) {
        py::handle el = list[i];

        if (is_pystr(el)) {
            formula.append(el.cast<std::string>());
        } else if (is_pyint(el)) {
            formula.append(var_names[el].cast<std::string>());
        } else if (is_pylist(el)) {
            formula.append(python_list_to_formula(el.cast<py::list>()));
        } else {
            throw std::invalid_argument("list elements must be integers or strings");
        }

        // separator between top-level elements
        if (i != list_size-1) formula.append(is_pylist(el) ? " ; " : " ");
    }

    return formula;
}

GretlMatrixPtr GretlDataset::hpfilt (const std::string &y, double hp_lambda, bool trend, bool one_sided)
{
    //
    // here we perform HP filtering
    // Note: we use hp_lambda instead of just lambda due to names clash on Python side
    //
    auto res = GretlMatrix::zeros(dset->n, 1);
    gretlopt opt = trend ? OPT_T : OPT_NONE;

    // we do the job
    if (one_sided) {
        err_ = oshp_filter(dset->Z[get_series_id(y)], res->get_ptr()->val, dset, hp_lambda, opt);
    } else {
        err_ = hp_filter(dset->Z[get_series_id(y)], res->get_ptr()->val, dset, hp_lambda, opt);
    }
    if (err_) handle_gretl_error(err_);

    return res;
}

GretlMatrixPtr GretlDataset::bwfilt (const std::string &y, int n, double omega)
{
    //
    // here we perform Butterworth filtering
    //
    auto res = GretlMatrix::zeros(dset->n, 1);

    // we do the job
    err_ = butterworth_filter(dset->Z[get_series_id(y)], res->get_ptr()->val, dset, n, omega);
    if (err_) handle_gretl_error(err_);

    return res;
}

GretlMatrixPtr GretlDataset::bkfilt (const std::string &y, int f1, int f2, int k)
{
    //
    // here we perform Baxter-King filtering
    //
    auto res = GretlMatrix::zeros(dset->n, 1);

    // we do the job
    err_ = bkbp_filter(dset->Z[get_series_id(y)], res->get_ptr()->val, dset, f1, f2, k);
    if (err_) handle_gretl_error(err_);

    return res;
}
