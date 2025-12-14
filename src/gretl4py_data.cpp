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
#include "gretl_calls.h"
#include <gretl/matrix_extra.h>
#include <gretl/gretl_model.h>
#include <gretl/genmain.h>
#include <gretl/objstack.h>
#include <gretl/gretl_drivers.h>
extern "C" {
    #include <gretl/var.h>
    #include <gretl/genfuncs.h>
    #include <gretl/gretl_xml.h>
    #include <gretl/gretl_typemap.h>
    #include <gretl/uservar.h>
    #include <gretl/transforms.h>
    #include <gretl/dataset.h>
    #include <gretl/system.h>
}
#include <algorithm>
#include <string>
#include <cassert>
#include <iomanip>
#include <random>
#include <filesystem>
#include <iostream>
#include <unordered_set>

#define F_CONTIGUOUS py::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_
#define is_NumPyArrayFC(m) ((m.flags() & F_CONTIGUOUS))

extern std::unordered_set<std::string> UserVars;

//////////////////////////////////////////////////
// static functions
//////////////////////////////////////////////////
static int get_int_from_python_dict (const py::dict &pydict,
                                     const char *key,
                                     int deflt = 1)
{
    int ret = deflt;

    if (pydict.contains(key)) {
        auto element = pydict[py::cast(key)];

        if (is_pyint(element)) {
            ret = py::cast<int>(element);
        }
    }

    return ret;
}

// two overloads for copy_strings_from_python_list()
static void copy_strings_from_python_list (const py::list &pylist, char **dest,
                                           int n_bytes, int src_offset = 0,
                                           int dst_offset = 0, bool trim = false)
{
    //
    // we copy the whole list to dest (length must be verified at the caller level)
    // with optional clearing
    //
    std::string str;
    int i = dst_offset;

    for (auto item : pylist) {
        str = std::string(py::str(item));
        if (trim) {
            str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
        }
        str.copy(dest[i++], n_bytes, src_offset);
    }
}

static void copy_strings_from_python_list (const py::list &pylist,
                                           std::string *dest, bool trim = false)
{
    //
    // we copy the whole list to dest (length must be verified at the caller level)
    // with optional clearing
    //
    int i = 0;
    std::string str;

    for (auto item : pylist) {
        str = std::string(py::str(item));
        if (trim) {
            str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
        }
        dest[i++] = str;
    }
}

static void numpy_array_to_gretl_storage (const py::dict &pydict, DATASET *dset)
{
    //
    // we copy "data_array" which lives in pydict to dset->Z
    //
    // we check if pydict["data_array"] is of <py::array_t> type
    if (!is_pynumarrayd(pydict[py::cast("data_array")])) {
        throw std::invalid_argument("Element 'data_array' of given dictionary is not of NumPy array type.");
    }

    NumPyArrayView arr_view;
    int err = 0;

    NumPyArray_extract(py::cast<py::array>(pydict[py::cast("data_array")]), arr_view);
    if (!arr_view.data) {
        throw std::runtime_error("NumPy array: null pointer.");
    }

    dset->n = arr_view.rows;
    dset->v = arr_view.cols + 1;
    err = allocate_Z(dset, OPT_NONE);
    if (err) {
        handle_gretl_error(err);
    }

    for (int i=1; i<dset->v; i++) {
        NumPyArray_to_gretl_matrix<double>(arr_view, dset->Z[i], i-1, true);
    }
    dset->t1 = 0;
    dset->t2 = dset->n-1;
}

template <typename T>
static void real_NumPyArray_to_gretl_matrix (const NumPyArrayView &view,
                                             double *dst, int offset, bool onecol)
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


//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////
// general purpose trim
std::string gretl4py_trim (std::string &s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                         [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());

    return s;
}

void NumPyArray_extract (const py::array &arr, NumPyArrayView &view)
{
    if (arr.ndim() > 2) {
        throw std::runtime_error("Given array has more than two dimensions.");
    }

    view.is_double = arr.dtype().is(py::dtype::of<double>());
    view.dtype = arr.dtype();
    view.base_array = arr;          // keep the NumPy array alive
    view.data = arr.request().ptr;
    view.rows = arr.shape(0);
    view.cols = arr.ndim() == 1 ? 1 : arr.shape(1);
    view.NumPyArrayFC = is_NumPyArrayFC(arr);

    const ssize_t itemsize = view.dtype.itemsize();
    const auto &strides = arr.strides();

    view.row_stride = strides[0] / itemsize;
    view.col_stride = (arr.ndim() == 1) ? 0 : strides[1] / itemsize;
}

template <typename T>
void NumPyArray_to_gretl_matrix (NumPyArrayView &view, T *dst,
                                 int offset, bool onecol)
{
    //
    // here we copy contents of some NumPy array described by &view
    // to storage available at *dst
    // offset here is used when we copy to DATASET
    //
    double *storage = NULL;

    if constexpr (std::is_same_v<T, gretl_matrix>) {
        storage = dst->val;
    } else {
        storage = dst;
    }

    if (view.is_double) {
        real_NumPyArray_to_gretl_matrix<double>(view, storage, offset, onecol);
    } else if (view.dtype.is(py::dtype::of<float>())) {
        real_NumPyArray_to_gretl_matrix<float>(view, storage, offset, onecol);
    } else if (view.dtype.is(py::dtype::of<int>())) {
        real_NumPyArray_to_gretl_matrix<int>(view, storage, offset, onecol);
    } else if (view.dtype.is(py::dtype::of<int64_t>())) {
        real_NumPyArray_to_gretl_matrix<int64_t>(view, storage, offset, onecol);
    } else {
        throw std::runtime_error("Unsupported NumPy dtype for conversion.");
    }
}
template void NumPyArray_to_gretl_matrix<gretl_matrix>(NumPyArrayView&, gretl_matrix*,
                                                       int, bool);


GretlTypesPtr convert_GretlType_to_GretlTypes (GretlType t, void *gr_obj_ptr,
                                               const int n, int &err)
{
    //
    // function converts libgretl's GretlType to gretl4py's GretlTypes
    // *gr_obj_ptr: pointer to libgretl's object
    // n - GRETL_TYPE_SERIES special case (series length)
    //
    auto res = std::make_unique<GretlTypes>();

    switch (t) {
    case GRETL_TYPE_NONE: {
        py::print("gretl_grab: didn't get anything!\n");
        *res = py::none();
        break;
    }
    case GRETL_TYPE_VOID: {
        *res = py::none();
        break;
    }
    case GRETL_TYPE_INT: {
        *res = *(static_cast<int*>(gr_obj_ptr));
        break;
    }
    case GRETL_TYPE_DOUBLE:
        *res = *(static_cast<double*>(gr_obj_ptr));
        break;
    case GRETL_TYPE_STRING: {
        *res = static_cast<char*>(gr_obj_ptr);
        break;
    }
    case GRETL_TYPE_MATRIX: {
        gretl_matrix *gretl_mat = static_cast<gretl_matrix*>(gr_obj_ptr);
        DNumPyArrayFC mat({gretl_mat->rows, gretl_mat->cols});

        std::memcpy(mat.mutable_data(), gretl_mat->val,
                    gretl_mat->rows * gretl_mat->cols * sizeof(double));
        if ((std::min)(gretl_mat->rows, gretl_mat->cols) == 1) { // this is to prevent 'min' macro expansion on MS SDK
            mat = mat.reshape({mat.size()});
        }
        *res = mat;
        break;
    }
    case GRETL_TYPE_BUNDLE: {
        *res = std::make_shared<GretlBundle>(GretlBundle(static_cast<gretl_bundle*>(gr_obj_ptr))); // as of 2025-08-15 we return GretlBundlePtr
        break;
    }
    case GRETL_TYPE_ARRAY:
    case GRETL_TYPE_MATRICES:
    case GRETL_TYPE_STRINGS:
    case GRETL_TYPE_BUNDLES: {
        gretl_array *a = static_cast<gretl_array*>(gr_obj_ptr);
        py::list py_list;
        GretlType tmp;

        for (int i=0; i<gretl_array_get_length(a); i++) {
            tmp = gretl_array_get_content_type(a);
            auto element = convert_GretlType_to_GretlTypes(tmp, gretl_array_get_element(a, i, &tmp, &err), n, err);
            py_list.append(variant_to_pyobject(std::move(*element)));
        }
        *res = py_list;
        break;
    }
    case GRETL_TYPE_SERIES: {
        double *s = static_cast<double*>(gr_obj_ptr);
        DNumPyArrayFC mat(n);

        std::memcpy(mat.mutable_data(), s, n * sizeof(double));
        *res = mat;
        break;
    }
    case GRETL_TYPE_LIST: {
        int *l = static_cast<int*>(gr_obj_ptr);
        py::list py_list;

        for (int i=1; i<=l[0]; i++) {
            py_list.append(l[i]);
        }
        *res = py_list;
        break;
    }
    default:
        py::print("gretl_grab: got unsupported type:", gretl_type_get_name(t));
        *res = py::none();
    }

    return res;
}

// Here we move item to named container for given instance
template <typename PyWrapped, typename Item, typename ContainerType>
void move_to_python_container (PyWrapped &wrapped_obj, Item &&item,
                               const char *container, const std::string &key)
{
    py::gil_scoped_acquire gil;
    py::object obj = py::cast(wrapped_obj);

    // we check if given container exists, and if not we create it
    if (!py::hasattr(obj, container)) {
        if constexpr (std::is_same_v<ContainerType, py::list>) {
            obj.attr(container) = py::list();
        } else if constexpr (std::is_same_v<ContainerType, py::dict>) {
            obj.attr(container) = py::dict();
        } else {
            static_assert(always_false<ContainerType>, "move_to_python_container(): unknown Container type");
        }
    }
    py::object dst = obj.attr(container);

    // we move an item to the container
    if constexpr (std::is_same_v<ContainerType, py::list>) {
        if (!is_pylist(dst)) {
            throw std::runtime_error("attribute '" + std::string(container) + "' is not a list");
        }
        dst.cast<py::list>().append(std::forward<Item>(item));
    } else if constexpr (std::is_same_v<ContainerType, py::dict>) {
        if (!is_pydict(dst)) {
            throw std::runtime_error("attribute '" + std::string(container) + "' is not a dictionary");
        }
        dst.cast<py::dict>()[py::cast(key)] = std::forward<Item>(item);
    } else {
        static_assert(always_false<ContainerType>, "move_to_python_container(): unknown Container type");
    }
}
template void move_to_python_container<GretlModel, py::dict, py::list>
    (GretlModel&, py::dict&&, const char*, const std::string&);

// Note: this is needed to avoid memory leaks when
// transferring GretlTypes to Python inside py::dict
py::object variant_to_pyobject (GretlTypes &&val)
{
    return std::visit([](auto &&arg) -> py::object
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_base_of_v<py::object, T>) {
            return std::move(arg);  // steal py::dict, py::list, etc.
        } else {
            return py::cast(std::move(arg));  // move std::string, etc.
        }
    }, std::move(val));
}

PythonArgInfo inspect_pyobject (py::handle obj)
{
    //
    // here we check what kind of argument arrived from python
    // and produce a struct with meta information
    //
    PythonArgInfo info;
    info.obj = obj;

    if (obj.is_none()) {
        info.gtype = GRETL_TYPE_NONE;
        info.pytype = "NoneType";
        return info;
    }

    if (is_pybool(obj)) {
        info.gtype = GRETL_TYPE_BOOL;
        info.pytype = "bool";
        return info;
    }

    if (is_pyint(obj)) {
        info.gtype = GRETL_TYPE_INT;
        info.pytype = "int";
        return info;
    }

    if (is_pyfloat(obj)) {
        info.gtype = GRETL_TYPE_DOUBLE;
        info.pytype = "float";
        return info;
    }

    if (is_pystr(obj)) {
        info.gtype = GRETL_TYPE_STRING;
        info.pytype = "str";
        return info;
    }

    if (is_pyGretlBundle(obj)) {
        info.gtype = GRETL_TYPE_BUNDLE;
        info.pytype = "GretlBundle";
        return info;
    }

    if (is_pynumarray(obj)) {
        info.gtype = GRETL_TYPE_MATRIX;
        info.pytype = "ndarray";
        auto arr = py::cast<py::array>(obj);
        info.shape.assign(arr.shape(), arr.shape() + arr.ndim());
        return info;
    }

    if (is_pytuple(obj) || is_pylist(obj)) {
        info.gtype = GRETL_TYPE_LIST;
        info.pytype = py::isinstance<py::tuple>(obj) ? "tuple" : "list";
        auto seq = py::reinterpret_borrow<py::sequence>(obj);
        info.shape.push_back(seq.size());
        return info;
    }

    info.gtype = GRETL_TYPE_ANY;
#if PYBIND11_VERSION_MAJOR < 3
    info.pytype = py::str(obj.get_type());
#else
    info.pytype = py::str(py::type::of(obj));
#endif
    info.valid = false;

    return info;
}

template <typename T>
void load_objects_from_dict (const py::dict &dict, const char *key, T &target)
{
    //
    // function for loading complex objects (lists, dicts from kwargs)
    //
    if (!dict.contains(key)) return;

    auto obj = dict[key];

    if constexpr (std::is_same_v<T, std::vector<std::string>>) {
        if (!is_pylist(obj)) {
            throw std::invalid_argument(std::string("'") + key + "' must be a list");
        }
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (!is_pystr(obj)) {
            throw std::invalid_argument(std::string("'") + key + "' must be a string");
        }
    } else {
        static_assert(always_false<T>, "load_objects_from_dict(): unknown type");
    }

    target = py::cast<T>(obj);
}
template void load_objects_from_dict<std::vector<std::string>>(const py::dict&, const char*,
                                                               std::vector<std::string>&);
template void load_objects_from_dict<std::string>(const py::dict&, const char*, std::string&);

//////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////

//
//GretlDataset class methods
//
GretlDataset::GretlDataset (const std::string &fname,
                            const py::dict &kwargs)
                            : id_(next_obj_id_++)
{
    //
    // load gretl data from file
    //
    std::string full_path = "";

    // we look for 'frompkg' option
    if (py::len(kwargs) > 0) {
        for (const auto &[key, val] : kwargs) {
            auto key_str = key.cast<std::string>();

            if (key_str == "frompkg" && is_pystr(val)) {
                auto val_str = val.cast<std::string>();

                auto pkg = load_package(val_str);
                full_path = pkg->get_examples_path(fname);
            } else {
                throw std::invalid_argument("unsupported option: '" + key_str + "'");
            }
        }
    }

    allocate_dset_();
    if (dset) {
        if (full_path.empty()) {
            err = gretl_get_data(fname.c_str(), dset, OPT_NONE, NULL);
        } else {
            err = gretl_get_data(full_path.c_str(), dset, OPT_NONE, NULL);
        }
    } else {
        err = E_ALLOC;
    }
    if (err) {
        destroy_dataset(dset);
        handle_gretl_error(err);
    }
}

GretlDataset::GretlDataset (const py::dict &pydict) : id_(next_obj_id_++)
{
    //
    // alternative initializer: we fill a gretl DATASET with the content of @pydict
    //
    gretlopt opt = OPT_NONE;
    py::list pylist;
    std::string *strBUF;
    int pd;

    allocate_dset_();

    // we fill up dset->Z
    numpy_array_to_gretl_storage(pydict, dset);

    if (dataset_allocate_varnames(dset) || dataset_allocate_obs_markers(dset)) {
        throw std::runtime_error("Dataset strings allocation failed.");
    }

    // we fill up dset->S
    pylist = pydict[py::cast("obs_labels")];
    if (pylist.size() != static_cast<long unsigned int>(dset->n)) {
        throw std::invalid_argument("Number of obs_labels is different than number of observations.");
    }
    copy_strings_from_python_list(pylist, dset->S, OBSLEN-1, 0, 0, true);

    // we fill up dset->varname
    pylist = pydict[py::cast("varname")];
    if (pylist.size() != static_cast<long unsigned int>(dset->v-1)) {
        throw std::invalid_argument("Number of varnames is different than number of variables.");
    }
    strBUF = new std::string[dset->v];
    copy_strings_from_python_list(pylist, strBUF);
    for (int i=0; i<dset->v-1; i++) {
        std::memset(dset->varname[i+1], '\0', VNAMELEN);
        gretl_normalize_varname(dset->varname[i+1], strBUF[i].c_str(), 1, 0);
    }
    delete []strBUF;

    // The following actions work fine if the python dict we're working with
    // was created by GretlDataset::to_dict (that is, starting from a gretl
    // dataset in libgretl's memory-space).
    pd = get_int_from_python_dict(pydict, "periodicity");
    dset->structure = get_int_from_python_dict(pydict, "structure", 0);
    opt = setobs_opt_from_structure_(dset->structure);
    set_obs(std::to_string(pd).c_str(), dset->S[0], dset, opt);
}

GretlDataset::GretlDataset (const GretlDataset &src) : id_(next_obj_id_++)
{
    //
    // copy constructor
    //
    if (!src.dset) {
        handle_gretl_error(E_DATA);
    }

    allocate_dset_();
    if (!dset) {
        handle_gretl_error(E_ALLOC);
    }

    // simple types
    dset->v = src.dset->v;
    dset->n = src.dset->n;
    dset->pd = src.dset->pd;
    dset->structure = src.dset->structure;
    dset->sd0 = src.dset->sd0;
    dset->t1 = src.dset->t1;
    dset->t2 = src.dset->t2;
    dset->markers = src.dset->markers;
    //dset->modflag = src.dset->modflag;    //FIXME: shall we copy this as well?
    dset->rseed = src.dset->rseed;
    dset->auxiliary = src.dset->auxiliary;
    dset->n_varinfo = src.dset->n_varinfo;
    dset->panel_pd = src.dset->panel_pd;
    dset->panel_sd0 = src.dset->panel_sd0;
    std::strncpy(dset->stobs, src.dset->stobs, OBSLEN);
    std::strncpy(dset->endobs, src.dset->endobs, OBSLEN);

    // we allocate memory
    err = allocate_Z(dset, OPT_NONE);
    if (err) {
        handle_gretl_error(err);
    }
    err = dataset_allocate_varnames(dset);
    if (err) {
        handle_gretl_error(err);
    }
    err = dataset_allocate_obs_markers(dset);
    if (err) {
        handle_gretl_error(err);
    }

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

    // we mark it as copy of ...
    if (!src.source.empty()) {
        source = src.source + " (copy of " + std::to_string(src.get_id()) + ")";
    } else {
        source = "copy of dataset " + std::to_string(src.get_id());
    }
}

GretlDataset::~GretlDataset (void)
{
    if (is_default) {
        DSET = nullptr;
    }
    free_dset_();
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
        dset = NULL;
    }
}

void GretlDataset::add_or_replace_allocated_series_ (gretl_double_ptr data,
                                                     const std::string &name,
                                                     int n, gretl_gchar_ptr label)
{
    //
    // Here we add or replace allocated series to dataset
    // Note: following libgretl convension, data is grabbed
    // only if there is no error
    //
    // we do basic checks
    if (n != dset->n) {
        throw std::invalid_argument("'" + name + "' is of wrong length: is n="
                                    + std::to_string(n) + ", should be n="
                                    + std::to_string(dset->n));
    }
    if (name.empty()) {
        throw std::invalid_argument("empty variable name");
    }

    // we check if we add new or replace series
    int v = get_series_id(name, false);

    // actual work
    double *data_ptr = data.release();

    if (v > 0) {
        err = dataset_replace_series(dset, v, data_ptr,
                                     label.get(), DS_GRAB_VALUES);
    } else {
        err = dataset_add_allocated_series(dset, data_ptr);
    }
    if (err) {
        std::free(data_ptr);
        handle_gretl_error(err);
    }

    if (v > 0) {
        py::print("Replaced series " + name + " (ID "
                  + std::to_string(v) + ")");
    } else {
        // we setup name and description for new series
        v = dset->v - 1;
        err = rename_series(dset, v, name.c_str(), OPT_NONE);
        if (err) {
            handle_gretl_error(err);
        }
        gretl_series_set_label(v, label.get());
    }
}

int GretlDataset::get_series_id (const std::string &series, const bool verbose) const
{
    //
    // method returns id of given series (in dset context)
    //
    int var_num = -1;

    var_num = current_series_index(dset, series.c_str());
    if (var_num < 0 && verbose) {
        throw std::invalid_argument("'" + series + "': series does not exist");
    }

    return var_num;
}

bool GretlDataset::is_panel (void) const
{
    return static_cast<bool>(dataset_is_panel(dset));
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
    if (dset->S != NULL) {
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

void GretlDataset::to_file (const std::string &fname,
                            const py::kwargs &kwargs)
{
    //
    // we write gretl data to file named by @fname
    //
    gretl_list_ptr varlist{nullptr};
    opt_ = OPT_NONE;

    // we handle options
    if (py::len(kwargs) > 0) {
        for (const auto &item : kwargs) {
            const std::string &key = item.first.cast<std::string>();

            if (key == "vars") {
                if (!is_pystr(item.second)) {
                    throw std::invalid_argument("'vars' must be a string");
                }
                auto val_str = py::cast<std::string>(item.second);

                varlist = list_from_formula(val_str, dset, err);
                if (err) {
                    handle_gretl_error(err);
                }
                continue;
            } else {
                gretl_valid_long_opt(opt_, STORE, key, item.second);
            }
        }
    }

    // we do the job
    if (opt_ & OPT_A) {
        err = write_matrix_as_dataset(fname.c_str(), opt_,
                                      GP_.prn_ptr());
    } else {
        err = write_data(fname.c_str(), varlist.get(),
                         dset, opt_, GP_.prn_ptr());
    }
    if (err) {
        handle_gretl_error(err);
    }
    GP_.print();
}

char *GretlDataset::add_series (gretl_double_ptr data,
                                const std::string &name,
                                int n, gretl_gchar_ptr label)
{
    //
    // Here we add existing series to dataset
    // Note: gretl_double_ptr and gretl_gchar_ptr are grabbed by dset
    //
    add_or_replace_allocated_series_(std::move(data), name,
                                     n, std::move(label));

    return dset->varname[get_series_id(name, false)];
}

void GretlDataset::add_series (const py::array &data,
                               const std::string &name)
{
    //
    // Here we add existing py::array to dataset
    // Note: the underline data is copied (deep copy)
    //
    NumPyArrayView view;

    NumPyArray_extract(data, view);
    if ((std::min)(view.cols, view.rows) > 1) {
        throw std::invalid_argument("given variable is not a vector");
    }
    int n = (std::max)(view.cols, view.rows);

    // we use std::malloc since memory will be deallocated by libgretl
    gretl_double_ptr data_copy(static_cast<double*>(std::malloc(n * sizeof(double))));

    // actual work
    NumPyArray_to_gretl_matrix<double>(view, data_copy.get(), 0, true);
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

template <GretlCmdIndex CI>
void GretlDataset::transforms (const std::string &vars,
                               const py::kwargs &kwargs)
{
    //
    // interface to several gretl's commands (CI)
    // which underlying engines live in tranforms.c
    //
    opt_ = OPT_NONE;

    // we set opts
    if (py::len(kwargs) > 0) {
        for (auto &item : kwargs) {
            if (!is_pystr(item.first)) {
                throw std::invalid_argument("option key must be a string");
            }
            const std::string &key = item.first.cast<std::string>();
            gretl_valid_long_opt(opt_, CI, key, item.second);
        }
    }

    // we construct list of variables
    auto varlist = list_from_formula(vars, dset, err);
    if (err) handle_gretl_error(err);

    // we do the job
    int *varlist_raw_ptr = varlist.release();

    if constexpr (CI == SQUARE) {
        err = list_xpxgenr(&varlist_raw_ptr, dset, opt_);
    } else if constexpr (CI == DUMMIFY) {
        err = list_dumgenr(&varlist_raw_ptr, dset, opt_);
    } else if constexpr (CI == DISCRETE) {
        err = list_makediscrete(varlist_raw_ptr, dset, opt_);
    } else if constexpr (CI == STDIZE) {
        err = list_stdgenr(varlist_raw_ptr, dset, opt_);
    }
    varlist.reset(varlist_raw_ptr);
    if (err) handle_gretl_error(err);

    // based on interact.c
    maybe_list_series(dset, GP_.prn_ptr());
    set_dataset_is_changed(dset, 1);

    GP_.print();
}
template void GretlDataset::transforms<SQUARE>(const std::string&, const py::kwargs&);
template void GretlDataset::transforms<DUMMIFY>(const std::string&, const py::kwargs&);
template void GretlDataset::transforms<DISCRETE>(const std::string&, const py::kwargs&);
template void GretlDataset::transforms<STDIZE>(const std::string&, const py::kwargs&);

template <GretlCmdIndex CI>
void GretlDataset::transforms_no_opts (const std::string &vars)
{
    //
    // interface to several gretl's commands (CI)
    // which underlying engines live in tranforms.c
    // Note: here we deal with commands which do not take arguments
    //
    gretl_list_ptr varlist{nullptr};

    // we construct list of external variables
    varlist = list_from_formula(vars, dset, err);
    if (err) handle_gretl_error(err);

    // we do the job
    int *varlist_raw_ptr = varlist.release();

    if constexpr (CI == DIFF || CI == LDIFF || CI == SDIFF) {
        err = list_diffgenr(varlist_raw_ptr, CI, dset);
    } else if constexpr (CI == LOGS) {
        err = list_loggenr(varlist_raw_ptr, dset);
    } else if constexpr (CI == ORTHDEV) {
        err = list_orthdev(varlist_raw_ptr, dset);
    }
    varlist.reset(varlist_raw_ptr);
    if (err) handle_gretl_error(err);

    // based on interact.c
    maybe_list_series(dset, GP_.prn_ptr());
    set_dataset_is_changed(dset, 1);

    GP_.print();
}
template void GretlDataset::transforms_no_opts<DIFF>(const std::string&);
template void GretlDataset::transforms_no_opts<LDIFF>(const std::string&);
template void GretlDataset::transforms_no_opts<SDIFF>(const std::string&);
template void GretlDataset::transforms_no_opts<LOGS>(const std::string&);
template void GretlDataset::transforms_no_opts<ORTHDEV>(const std::string&);

std::string GretlDataset::repr (void) const
{
    std::string ret = "GretlDataset, source " + source;

    auto desc = gretl_get_gdt_description(source.c_str(), &err);
    if (!err) {
        ret.append("\n").append(desc);
        std::free(desc);
    }

    return ret;
}

void GretlDataset::print (const py::kwargs &kwargs)
{
    gretl_list_ptr var_list{nullptr};
    bool summary = false;
    bool byobs = false;
    bool range = false;
    std::string val_str = "";
    opt_ = OPT_NONE;

    for (auto &item : kwargs) {
        // ensure the key is a string
        if (!is_pystr(item.first)) {
            throw std::invalid_argument("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        // we use ::to_string() to get an owned buffer
        val_str = MapPyToCpp(item.second).to_string();

        if (key == "summary") {
            summary = (val_str == "1");
        } else if (key == "byobs") {
            byobs = (val_str == "1");
        } else if (key == "byvar") {
            if (val_str == "1") {
                summary = byobs = false;
            }
        } else if (key == "series") {
            // note: we print only specified variables
            var_list = list_from_formula(val_str, dset, err);
            if (err) {
                handle_gretl_error(err);
            }
        } else if (key == "range") {
            auto pos = val_str.find(':');

            if (pos != std::string::npos && val_str.find(':', pos + 1) == std::string::npos) {
                gretl_valid_long_opt(opt_, PRINT, "range", py::handle(), val_str.c_str());
                range = true;
            } else {
                throw std::invalid_argument("range: only one ':' is allowed");
            }
        } else {
            throw std::invalid_argument("print: invalid key '" + key + "'");
        }
    }

    if (summary) {
        auto *prn = GP_.prn_ptr();

        if (dataset_is_time_series(dset)) {
            char s1[OBSLEN], s2[OBSLEN];

            ntolabel(s1, 0, dset);
            ntolabel(s2, dset->n-1, dset);
            pprintf(prn, "Time series, frequency %d, T = %d, %s to %s\n\n",
                    dset->pd, dset->n, s1, s2);
        } else if (is_panel()) {
            pprintf(prn, "Panel data, N = %d, T = %d\n\n",
                    dset->n / dset->pd, dset->pd);
        } else {
            pprintf(prn, "Cross-sectional data, N = %d\n\n", dset->n);
        }
        list_series(dset, OPT_NONE, prn);
    } else {
        opt_ = byobs ? OPT_O : OPT_NONE;
        if (range) {
            SET_OPT(opt_, OPT_R);
        }
        err = printdata(var_list.get(), NULL, dset, opt_, GP_.prn_ptr());
        if (err) {
            handle_gretl_error(err);
        }
    }
    GP_.print();
}

void GretlDataset::sample (const py::kwargs &kwargs)
{
    //
    // A method to mimic gretl's 'smpl' command
    //
    std::string t1str, t2str, restr;
    bool opt_replace = false;
    bool full = false;
    bool unit = false;
    bool time = false;
    bool quiet = false;

    opt_ = OPT_NONE;
    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::invalid_argument("option key must be a string");
        }
        const std::string key = item.first.cast<std::string>();
        const auto val_str = MapPyToCpp(item.second).to_string();

        if (key == "t1") {
            t1str = MapPyToCpp(item.second).to_string();
        } else if (key == "t2") {
            t2str = MapPyToCpp(item.second).to_string();
        } else if (key == "restriction") {
            restr = MapPyToCpp(item.second).to_string();
        } else if (key == "replace") {
            opt_replace = item.second.cast<bool>();
        } else if (key == "full") {
            full = item.second.cast<bool>();
        } else if ((key == "unit" || key == "time") && is_panel()) {
            if (key == "unit") {
                unit = item.second.cast<bool>();
            } else {
                time = item.second.cast<bool>();
            }
        } else if (key == "quiet") {
            quiet = item.second.cast<bool>();
        } else {
            throw std::invalid_argument("invalid key '" + key + "' in 'sample' args");
        }
    }

    // reject invalid cases (mutually exclusive)
    if (full && (!t1str.empty() || !t2str.empty() || opt_replace)) {
        throw std::invalid_argument("sample: invalid use of 'full'");
    } else if (!restr.empty() && (!t1str.empty() || !t2str.empty())) {
        throw std::invalid_argument("sample: invalid use of 'restriction'");
    } else if (opt_replace && restr.empty()) {
        throw std::invalid_argument("sample: invalid use of 'replace'");
    } else if (is_panel() && (unit && time)) {
        throw std::invalid_argument("sample: 'unit' and 'time' options are mutually exclusive");
    }
    bool have_args = (!t1str.empty() || !t2str.empty() || !restr.empty() || opt_replace || full || unit || time);

    if (!have_args) {
        // just report current sample
        print_smpl(dset, get_full_length_n(), OPT_NONE, GP_.prn_ptr());
    } else if (full) {
        // "smpl full"
        err = restore_full_sample(dset, NULL);
    } else if (!restr.empty()) {
        // smpl <restr> --restrict [--replace]
        opt_ = opt_replace ? static_cast<gretlopt>(OPT_R | OPT_P) : OPT_R;
        err = restrict_sample(restr.data(), NULL, dset, NULL,
                              opt_, GP_.prn_ptr(), NULL);
    } else {
        // use empty string as fallback when only one of t1/t2 is set (smpl <t1str> <t2str>)
        std::string ts1 = t1str.empty() ? ";" : t1str;
        std::string ts2 = t2str.empty() ? ";" : t2str;

        if (is_panel()) {
            // panel data: --unit or --time option
            if (unit) {
                SET_OPT(opt_, OPT_U);
            }
            if (time) {
                SET_OPT(opt_, OPT_X);
            }
            err = set_panel_sample(ts1.c_str(), ts2.c_str(), opt_,
                                   dset, NULL, GP_.prn_ptr());
        } else {
            err = set_sample(ts1.c_str(), ts2.c_str(), dset, OPT_NONE);
        }
    }
    if (err) {
        handle_gretl_error(err);
    }

    if (have_args) {
        print_smpl(dset, get_full_length_n(), OPT_NONE, GP_.prn_ptr());
    }
    if (!quiet) {
        GP_.print();
    }
}

template <typename T>
GretlDatasetPtr GretlDataset::getitem_impl (const T &slice)
{
    //
    // this method is an underlying engine for '[]' operator
    //
    py::dict opts;
    std::string slice_str;
    gretl_list_ptr drop_list{nullptr};

    // we parse slice / list / string
    if constexpr (std::is_same_v<T, py::slice>) {
        py::object start = slice.attr("start");
        py::object stop = slice.attr("stop");
        int t1 = py::isinstance<py::none>(start) ? 0 : start.cast<int>();
        int t2 = py::isinstance<py::none>(stop) ? 0 : stop.cast<int>();
        opts["t1"] = t1;
        opts["t2"] = t2;
        slice_str = std::to_string(t1) + ":" + std::to_string(t2);
    } else if constexpr (std::is_same_v<T, py::list>) {
        std::ostringstream oss;

        // we construct list of variables to retain
        for (size_t i=0; i<slice.size(); ++i) {
            oss << py::str(slice[i]).cast<std::string>();
            if (i + 1 < slice.size()) {
                oss << " ";
            }
        }
        slice_str = oss.str();
        gretl_list_ptr slice_list = list_from_formula(slice_str, dset, err);
        if (err) handle_gretl_error(err);

        // we construct list of variables to drop
        gretl_list_ptr full_list(full_var_list(dset, NULL));
        drop_list = gretl_list_ptr(gretl_list_drop(full_list.get(),
                                                   slice_list.get(),
                                                   &err));
        if (err) handle_gretl_error(err);
    } else if constexpr (std::is_same_v<T, std::string>) {
        auto pos = slice.find(':');

        // at first we check if we deal with range slicer
        if (pos != std::string::npos && slice.find(':', pos + 1) == std::string::npos) {
            opts["t1"] = slice.substr(0, pos);
            opts["t2"] = slice.substr(pos + 1);
        } else {
            opts["restriction"] = slice;
        }
        slice_str = slice;
    } else {
        static_assert(always_false<T>, "GretlDataset::getitem_impl: unknown slicing rule type");
    }

    // me make deep copy of current instance
    auto RES = std::make_shared<GretlDataset>(*this);

    // we do the job
    if constexpr (std::is_same_v<T, py::list>) {
        // dropping variables
        err = gretl_delete_variables(drop_list.get(), NULL, OPT_F,
                                     RES->dset, NULL, GP_.prn_ptr());
    } else {
        // for sampling
        opts["quiet"] = true;
        RES->sample(opts);
        err = perma_sample(RES->dset, OPT_T, GP_.prn_ptr(), NULL);
    }
    if (err) {
        GP_.print();
        handle_gretl_error(err);
    }
    RES->source += " slice: " + slice_str;

    return RES;
}
template GretlDatasetPtr GretlDataset::getitem_impl<py::slice>(const py::slice&);
template GretlDatasetPtr GretlDataset::getitem_impl<py::list>(const py::list&);
template GretlDatasetPtr GretlDataset::getitem_impl<std::string>(const std::string&);

py::object GretlDataset::corr (const std::string &vars, const py::kwargs &kwargs)
{
    //
    // general interface for computing correlations
    //
    gretl_list_ptr var_list{nullptr};

    // we construct list of variables (if needed)
    if (!vars.empty()) {
        var_list = list_from_formula(vars, dset, err);
        if (err) {
            handle_gretl_error(err);
        }
    }

    opt_ = OPT_NONE;
    // now we loop over kwargs;
    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::invalid_argument("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "plot") {
            throw std::invalid_argument("invalid option flag: " + key);
        } else if (key == "matrix") {
            throw std::invalid_argument("matrix: not supported yet");
        } else {
            gretl_valid_long_opt(opt_, CORR, key, item.second);
        }
    }

    // we switch off gnuplot
    SET_OPT(opt_, OPT_U);
    gretl_valid_long_opt(opt_, CORR, "plot", py::handle(), "none");

    // actual work
    if (opt_ & OPT_K) {
        err = kendall_tau(var_list.get(), dset, opt_, GP_.prn_ptr());
    } else if (opt_ & OPT_S) {
        err = spearman_rho(var_list.get(), dset, opt_, GP_.prn_ptr());
    } else {
        err = gretl_corrmx(var_list.get(), dset, opt_, GP_.prn_ptr());
    }
    if (err) {
        handle_gretl_error(err);
    }

    GP_.print();

    return get_accessor("$result");
}

py::object GretlDataset::cov (const std::string &vars)
{
    //
    // we compute covariance for given variables
    //
    gretl_list_ptr var_list{nullptr};
    gretl_matrix_ptr mat{nullptr};

    // we construct list of variables
    if (vars.empty()) {
        var_list.reset(full_var_list(dset, NULL));
    } else {
        var_list = list_from_formula(vars, dset, err);
        if (err) {
            handle_gretl_error(err);
        }
    }

    // actual work
    mat.reset(gretl_covariance_matrix_from_varlist(var_list.get(),
                                                   dset, NULL, &err));
    if (err) {
        handle_gretl_error(err);
    }

    auto res = convert_GretlType_to_GretlTypes(GRETL_TYPE_MATRIX,
                                               mat.get(), 0, err);

    return variant_to_pyobject(std::move(*res));
}

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

    if (argc < 2) {
        throw std::invalid_argument("too few arguments for 'setobs'");
    }

    if (pcount > 0) {
        if (rcount > 0 || opt_ != OPT_NONE) {
            throw std::invalid_argument("invalid mixed modes in 'setobs'");
        }
        // equiv. to --panel-vars
        opt_ = OPT_P;
    }

    err = set_obs(arg1.data(), arg2.data(), dset, opt_);
    if (err) {
        handle_gretl_error(err);
    }
}

void GretlDataset::set_as_default (void)
{
    if (DSET) {
        DSET->is_default = false;
    }
    DSET = shared_from_this();
    is_default = true;
}

int GretlDataset::get_id (void) const
{
    return id_;
}

void GretlDataset::gretl_series_set_label (const int number,
                                           char *label)
{
    series_set_label(dset, number, label);
}

void GretlDataset::lags (const std::string &vars, int order)
{
    //
    // interface to libgretl's list_laggenr
    //
    gretl_list_ptr varlist{nullptr};

    // we construct list of external variables
    varlist = list_from_formula(vars, dset, err);
    if (err) {
        handle_gretl_error(err);
    }

    // we do the job
    int *varlist_raw_ptr = varlist.release();
    err = list_laggenr(&varlist_raw_ptr, 1, order, NULL, dset, 0, OPT_NONE);
    varlist.reset(varlist_raw_ptr);
    if (err) {
        handle_gretl_error(err);
    }

    // based on interact.c
    maybe_list_series(dset, GP_.prn_ptr());
    set_dataset_is_changed(dset, 1);

    GP_.print();
}

py::list GretlDataset::varnames (void) const
{
    py::list names;

    for (int i=0; i<dset->v; i++) {
        names.append(dset->varname[i]);
    }

    return names;
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

gretlopt GretlDataset::setobs_opt_from_structure_ (int structure)
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
}

py::object GretlDataset::get_accessor (const std::string &accessor) const
{
    auto res = gretl_generate_gretl_object(accessor.c_str(), err,
                                           GRETL_OBJ_DSET, dset);
    if (err) {
        handle_gretl_error(err);
    }

    return variant_to_pyobject(std::move(*res));
}

void GretlDataset::test (const std::string &type, const std::string &vars,
                         const py::kwargs &kwargs)
{
    //
    // this method performs test which do take a variables list
    //
    py::dict res;
    int order = 0;
    gretl_list_ptr ex_var{nullptr};

    // we construct list of external variables
    ex_var = list_from_formula(vars, dset, err);
    if (err) {
        handle_gretl_error(err);
    }

    // we set up options
    unpack_kwargs_test(opt_, type, kwargs, order);

    // we perform the test
    if (type == "adf") {
        err = adf_test(order, ex_var.get(), dset, opt_, GP_.prn_ptr());
    } else if (type == "bds") {
        err = bds_test_driver(order, ex_var.get(), dset, opt_, GP_.prn_ptr());
    } else if (type == "coint") {
        err = engle_granger_test(order, ex_var.get(), dset, opt_, GP_.prn_ptr());
    } else if (type == "corrgm") {
        err = corrgram(ex_var.get()[1], order, 0, dset, opt_, GP_.prn_ptr());
    } else if (type == "difftest") {
        err = diff_test(ex_var.get(), dset, opt_, GP_.prn_ptr());
    } else if (type == "johansen") {
        err = johansen_test_simple(order, ex_var.get(), dset, opt_, GP_.prn_ptr());
    } else if (type == "kpss") {
        err = kpss_test(order, ex_var.get(), dset, opt_, GP_.prn_ptr());
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
            err = llc_test_driver(mat.str().c_str(), ex_var.get(),
                                  dset, opt_, GP_.prn_ptr());
        } else {
            err = llc_test_driver(std::to_string(order).c_str(), ex_var.get(),
                                  dset, opt_, GP_.prn_ptr());
        }
    } else if (type == "meantest") {
        err = means_test(ex_var.get(), dset, opt_, GP_.prn_ptr());
    } else if (type == "normtest") {
        if (ex_var.get()[0] > 1) {
            throw std::invalid_argument("normtest takes just one series");
        }
        err = gretl_normality_test(ex_var.get()[1], dset, opt_, GP_.prn_ptr());
    } else if (type == "runs") {
        if (ex_var.get()[0] > 1) {
            throw std::invalid_argument("runs takes just one series");
        }
        err = runs_test(ex_var.get()[1], dset, opt_, GP_.prn_ptr());
    } else if (type == "vartest") {
        if (ex_var.get()[0] != 2) {
            throw std::invalid_argument("vartest takes exactly two series");
        }
        err = vars_test(ex_var.get(), dset, opt_, GP_.prn_ptr());
    } else {
        throw std::invalid_argument(type + " is a model-based test");
    }

    if (err) {
        handle_gretl_error(err);
    }

    res[py::cast("test")] = get_accessor("$test");
    res[py::cast("pvalue")] = get_accessor("$pvalue");
    res[py::cast("summary")] = GP_.get_buffer();
    if (!kwargs.contains("silent")) {
        GP_.print();
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

    auto res = convert_GretlType_to_GretlTypes(GRETL_TYPE_SERIES,
                                               x, dset->n, err);
    if (err) {
        handle_gretl_error(err);
    }

    return variant_to_pyobject(std::move(*res));
}

DNumPyArrayFC GretlDataset::hpfilt (const std::string &series,
                                    double hp_lambda, bool trend,
                                    bool one_sided)
{
    //
    // here we perform HP filtering
    // Note: we use hp_lambda instead of just lambda due to names clash on Python side
    //
    DNumPyArrayFC res(dset->n);
    gretlopt opt = trend ? OPT_T : OPT_NONE;

    // we do the job
    if (one_sided) {
        err = oshp_filter(dset->Z[get_series_id(series)],
                          res.mutable_data(), dset, hp_lambda, opt);
    } else {
        err = hp_filter(dset->Z[get_series_id(series)],
                        res.mutable_data(), dset, hp_lambda, opt);
    }
    if (err) {
        handle_gretl_error(err);
    }

    return res;
}

DNumPyArrayFC GretlDataset::bwfilt (const std::string &series,
                                    int order, double omega)
{
    //
    // here we perform Butterworth filtering
    //
    DNumPyArrayFC res(dset->n);

    // we do the job
    err = butterworth_filter(dset->Z[get_series_id(series)],
                             res.mutable_data(), dset, order, omega);
    if (err) {
        handle_gretl_error(err);
    }

    return res;
}

DNumPyArrayFC GretlDataset::bkfilt (const std::string &series,
                                    int f1, int f2, int k)
{
    //
    // here we perform Baxter-King filtering
    //
    DNumPyArrayFC res(dset->n);

    // we do the job
    err = bkbp_filter(dset->Z[get_series_id(series)],
                      res.mutable_data(), dset, f1, f2, k);
    if (err) {
        handle_gretl_error(err);
    }

    return res;
}


//
// GretlBundle class methods
//
GretlBundle::GretlBundle (void) : b_(gretl_bundle_new())
{
    if (!b_) {
        handle_gretl_error(E_ALLOC);
    }
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

GretlBundle::GretlBundle (gretl_bundle *src) : free_b_(false)
{
    //
    // Here we make a copy of src
    // Note: 'src. must be deallocated by libgretl, otherwise there
    // will be a memory leak
    //
    b_ = gretl_bundle_copy(src, &err_);
    if (err_) handle_gretl_error(err_);
}

GretlBundle::~GretlBundle (void)
{
    if (free_b_ && b_) {
        gretl_bundle_destroy(b_);
    }
}

template <typename T>
void GretlBundle::extract_ (const T &t)
{
    //
    // extract the bundle and either register on python side
    // or store in as_dict
    //
    int n_keys = 0;

    char **keys = gretl_bundle_get_keys_raw(b_, &n_keys);
    if (!keys || n_keys <= 0) {
        throw std::runtime_error("didn't get any keys from bundle");
    }

    for (int i=0; i<n_keys; ++i) {
        const char *key = keys[i];

        // py::object will be registered as an attribute on python side
        if constexpr (std::is_same_v<T, py::object>) {
            py::setattr(t, key, get_element<py::object>(key));
        } else if constexpr (std::is_same_v<T, py::dict>) {
            as_dict_[py::cast(key)] = get_element<py::object>(key);
        } else {
            static_assert(always_false<T>, "GretlBundle::extract_(): unknown type");
        }
    }
    std::free(keys);
}

int GretlBundle::get_err (void) const
{
    return err_;
}

py::dict GretlBundle::to_dict (void)
{
    if (as_dict_.empty()) {
        extract_<py::dict>(py::dict());
    }

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
    if (err_) handle_gretl_error(err_);

    GretlPrint GP = GretlPrint();

    // we do the job
    if (tree) {
        err_ = gretl_bundle_print_tree(b_, GP.prn_ptr());
    } else {
        err_ = gretl_bundle_print(b_, GP.prn_ptr());
    }
    if (err_) handle_gretl_error(err_);

    // we either just print or return buffer
    if constexpr (std::is_same_v<T, void>) {
        GP.print();
    } else if constexpr (std::is_same_v<T, std::string>) {
        std::string summary = GP.get_buffer();
        return summary;
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

py::list GretlBundle::keys()
{
    //
    // we return bundle keys as py::list
    //
    py::list res;
    int n_keys = 0;

    char **keys = gretl_bundle_get_keys_raw(b_, &n_keys);
    if (!keys || n_keys <= 0) {
        throw std::runtime_error("didn't get any keys from bundle");
    }

    for (int i=0; i<n_keys; ++i) {
        res.append(keys[i]);
    }
    std::free(keys);

    return res;
}

template <typename T>
T GretlBundle::get_element (const std::string &key)
{
    //
    // we get bundle element by name (key)
    // and return either py::object or GretlTypesPtr
    //
    int n = 0;

    GretlType gtype = gretl_bundle_get_member_type(b_, key.c_str(), &err_);
    if (err_) handle_gretl_error(err_);

    void *data = gretl_bundle_get_target(b_, key.c_str(), &gtype, &n, &err_);
    if (err_) handle_gretl_error(err_);

    auto val = convert_GretlType_to_GretlTypes(gtype, data, n, err_);
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
    if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int>) {
        err_ = gretl_bundle_set_int(b_, key.c_str(), t);
    } else if constexpr (std::is_same_v<T, double>) {
        err_ = gretl_bundle_set_scalar(b_, key.c_str(), t);
    } else if constexpr (std::is_same_v<T, std::string>) {
        err_ = gretl_bundle_set_string(b_, key.c_str(), t.c_str());
    } else if constexpr (std::is_same_v<T, py::array>) {
        NumPyArrayView arr_view;
        gretl_matrix_ptr mat{nullptr};

        NumPyArray_extract(t, arr_view);
        if (!arr_view.data) {
            throw std::runtime_error("NumPy array: null pointer.");
        }
        mat.reset(gretl_matrix_alloc(arr_view.rows, arr_view.cols));

        NumPyArray_to_gretl_matrix<gretl_matrix>(arr_view, mat.get());
        err_ = gretl_bundle_donate_data(b_, key.c_str(), mat.release(), GRETL_TYPE_MATRIX, 0);
    } else if constexpr (std::is_same_v<T, GretlBundlePtr>) {
        err_ = gretl_bundle_set_data(b_, key.c_str(), t->get_ptr(), GRETL_TYPE_BUNDLE, 0);
    } else {
        static_assert(always_false<T>, "GretlBundle::set_element(): unknown type");
    }
    if (err_) handle_gretl_error(err_);
}
template void GretlBundle::set_element<bool>(const std::string&, const bool&);
template void GretlBundle::set_element<int>(const std::string&, const int&);
template void GretlBundle::set_element<double>(const std::string&, const double&);
template void GretlBundle::set_element<std::string>(const std::string&, const std::string&);
template void GretlBundle::set_element<py::array>(const std::string&, const py::array&);
template void GretlBundle::set_element<GretlBundlePtr>(const std::string&, const GretlBundlePtr&);

//
// MapPyToCpp class methods
//
MapPyToCpp::MapPyToCpp (const std::optional<py::object> &pyobject)
{
    convert_<py::object>(&*pyobject);
}

MapPyToCpp::MapPyToCpp (const py::handle &pyhandle)
{
    convert_<py::handle>(&pyhandle);
}

template <typename T>
void MapPyToCpp::convert_ (const T *pyobj)
{
#if GRETL_PYBIND11_DEBUG
    std::printf("converting %s to string\n",
                py::cast<std::string>(pyobj->get_type()).c_str());
# endif
    if (is_pystr(*pyobj)) {
        as_str_ = py::cast<std::string>(*pyobj);
    } else if (is_pyint(*pyobj) || is_pybool(*pyobj)) {
        as_str_ = std::to_string(py::cast<int>(*pyobj));
    } else if (is_pyfloat(*pyobj)) {
        as_str_ = std::to_string(py::cast<double>(*pyobj));
    } else {
        err = E_TYPES;
        as_str_.clear();
        std::cerr << "I don't know how to convert '";
#if PYBIND11_VERSION_MAJOR < 3
        std::cerr << py::cast<std::string>(pyobj->get_type());
#else
        std::cerr << py::cast<std::string>(py::type::of(*pyobj));
#endif // PYBIND11_VERSION_MAJOR
        std::cerr << "' to std::string" << std::endl;
    }
}

//
// GretlPackage class methods
//
GretlPackage::GretlPackage (const std::string &pkg_name,
                            const std::string &suffix)
                            : pkg_name_(pkg_name), suffix_(suffix)
{
    const std::unordered_set<UfunRole> excluded_roles = { UFUN_BUNDLE_PLOT,
            UFUN_GUI_MAIN, UFUN_GUI_PRECHECK, UFUN_PLOT_PRECHECK, UFUN_UI_MAKER };
    std::unordered_set<std::string> excluded_functions;
    std::ostringstream oss;
    char path[MAXLEN];
    int idxp = 0;

    // we try to find a path and load the package
    *path = '\0';
    if (suffix.empty()) {
        err = get_full_filename((pkg_name + ".gfn").c_str(), path, OPT_I);
    } else {
        err = get_full_filename((pkg_name + suffix).c_str(), path, OPT_I);
    }
    if (err) {
        handle_gretl_error(err);
    }
    pkg_ = get_function_package_by_filename(path, &err);
    if (err) {
        handle_gretl_error(err);
    }
    full_path_ = path;
    auto pos = full_path_.find_last_of("/\\");
    if (pos != std::string::npos) {
        base_dir_ = full_path_.substr(0, pos);
    } else {
        base_dir_ = "";
    }

    // we take info bundle
    err = bundle_function_package_info(full_path_.c_str(), GB_.get_ptr());
    if (err) {
        handle_gretl_error(err);
    }

    // setup some basic info about package
    if (pkg_name != function_package_get_name(pkg_)) {
        throw std::runtime_error("package name mismatch: internal: '"
                                 + std::string(function_package_get_name(pkg_))
                                 + "', provided: '" + pkg_name_ + "'");
    }
    function_names_init();

    // we look up for JEL codes
    jel_ = get_properties_<std::vector<std::string>>("tags");

    // we grab only public functions (names and indexes)
    // Note: we exclude functions with specific roles (mostly related with plotting and GUI)
    for (auto role : excluded_roles) {
        auto func_name = get_properties_<std::string>(package_role_get_key(role));
        if (!func_name.empty()) {
            excluded_functions.insert(std::move(func_name));
        }
    }

    // we register functions metadata
    while (const char* func_name = next_available_function_name(pkg_, &idxp)) {
        if (gretl_is_public_user_function(func_name) && !excluded_functions.count(func_name)) {
            // pointers
            auto func_ptr = get_function_from_package(func_name, pkg_);
            if (!func_ptr) {
                throw std::runtime_error("unable to get pointer to function: "
                                         + std::string(func_name));
            }
            func_ptrs_.emplace(func_name, std::move(func_ptr));
            // arguments
            std::map<std::string, int> arg_names_tmp;
            for (int i=0; i<fn_n_params(func_ptr); ++i) {
                arg_names_tmp.emplace(fn_param_name(func_ptr, i), i);
            }
            func_args_.emplace(func_name, std::move(arg_names_tmp));
        }
    }
    if (func_ptrs_.empty()) {
        throw std::runtime_error("no public functions found in '"
                                 + pkg_name + "' package");
    }

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
                if (it.first.find("estimate") != std::string::npos) {
                    set_main(it.first);
                }
            }
        }
    }
    check_if_defines_estimator_();

    // package summary
    oss << std::fixed << std::setprecision(2) << function_package_get_version(pkg_);
    py::print("We use '" + pkg_name_ + "' package, version: " + oss.str()
              + ", JEL codes: " + get_properties_<std::string>("tags")
              + " (" + get_properties_<std::string>("author") + ")"
              + "\n  main function: " + (main_func_.empty() ? "none" : main_func_)
              + ", provides estimator: " + (defines_estimator_ ? "yes" : "no"));
    if (main_func_.empty()) {
        py::print("warning: package main function didn't found: please use ::set_main()");
    }
    py::print();
}

GretlPackage::~GretlPackage (void)
{
    err = function_package_unload_full(pkg_name_.c_str());
    if (err) {
        std::cerr << errmsg_get_with_default(err) << std::endl;
    }
}

void GretlPackage::check_if_defines_estimator_ (void)
{
    const std::unordered_set<std::string> estimation_codes = {
        "C13", "C14", "C15", "C20", "C21", "C22", "C23", "C24",
        "C25", "C26", "C30", "C31", "C32", "C33", "C34", "C35",
        "C36", "C40", "C41", "C51", "C52", "C53", "C54", "C58"};

    for (const auto &it : jel_) {
        if (estimation_codes.count(it)) {
            defines_estimator_ = true;
            return;
        }
    }
}

template <typename T>
T GretlPackage::get_properties_ (const char *property)
{
    //
    // we get package properties
    //
    T res;
    gchar *val = nullptr;

    if constexpr (std::is_same_v<T, DataReq>) {
        err = function_package_get_properties(pkg_, property, &res, NULL);
    } else {
        err = function_package_get_properties(pkg_, property, &val, NULL);
    }
    if (err) handle_gretl_error(err);

    // we produce output
    if constexpr (std::is_same_v<T, DataReq>) {
        return res;
    } else if (!val) {
        return {}; // gracefully return empty value
    } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
        std::istringstream iss(val);
        std::string token;
        while (iss >> token) {
            res.push_back(std::move(token));
        }
    } else if constexpr (std::is_same_v<T, std::string>) {
        res = val;
    } else {
        static_assert(always_false<T>, "GretlPackage::get_properties_(): unknown type");
    }

    // we free results (id needed)
    g_free(val);

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
    if (py::len(kwargs) > 0) {
        // map function to call with its arguments
        std::map<std::string, int> func_name_args = get_func_args(func_name);

        // vector of func_name_args items ordered by value (int)
        std::vector<std::pair<std::string, int>> func_name_args_sorted(func_name_args.begin(),
                                                                       func_name_args.end());
        std::sort(func_name_args_sorted.begin(), func_name_args_sorted.end(),
          [](const auto &a, const auto &b)
          {
              return a.second < b.second; // sort by value
          });

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
                if (!func_name_args.count(key_str)) {
                    throw std::invalid_argument(key_str +
                                                ": there is no such argument in function: "
                                                + func_name);
                }
            }
        }

        // function-related options (arguments pushed on stack)
        std::string arg_name;   // variable to be pushed on stack (name)
        void *arg_value;        // pointer to variable's value to be pushed on stack
        GretlType arg_type;     // variable's type
        double darg_value;      // value to be pushed on stack (if double)

        for (const auto &[key, arg_no] : func_name_args_sorted) {
            if (!kwargs.contains(key)) {
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
                err = push_function_arg(func_call, arg_name.c_str(),
                                        NULL, GRETL_TYPE_USERIES, &vnum);
                if (err) {
                    handle_gretl_error(err);
                }
                continue;
            } else if (arginfo.gtype == GRETL_TYPE_BUNDLE) {
                // note: here we deal with both: GRETL_TYPE_BUNDLE and GRETL_TYPE_BUNDLE_REF
                // final distinction is made by key_type
                auto GB = val.cast<GretlBundlePtr>();
                err = push_function_arg(func_call, NULL, NULL,
                                        key_type, GB->get_ptr());
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
                if (UserVars.find(arg_name) != UserVars.end()) {
                    user_var *uv = get_user_var_by_name(arg_name.c_str());
                    // for reference, we change uv type!!!
                    arg_type = is_ref_user_var ?
                        gretl_type_get_ref_type(user_var_get_type(uv))
                        : user_var_get_type(uv);
                    err = push_function_arg(func_call, arg_name.c_str(), uv,
                                            arg_type, user_var_get_value(uv));
                    if (err) {
                        handle_gretl_error(err);
                    }
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
            err = push_function_arg(func_call, key.c_str(),
                                    NULL, arg_type, arg_value);
            if (err) {
                handle_gretl_error(err);
            }
        }
    }
}

std::string GretlPackage::repr_impl (void)
{
    err = print_function_package_info(full_path_.c_str(), 0, GP_.prn_ptr());
    if (err) {
        handle_gretl_error(err);
    }

    return GP_.get_buffer();
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
    if (!main_func_nd_.empty()) {
        main_func_nd_.clear();
    }
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

void GretlPackage::show_functions (void) const
{
    if (func_args_.empty()) {
        throw std::runtime_error("no available functions");
    }

    GretlType type;
    std::string type_str;
    ufunc *func_ptr;

    py::print("Functions available in " + pkg_name_ + " package:");
    for (const auto &[key, val] : func_args_) {
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
    if (func_args_.empty()) {
        throw std::runtime_error("no available functions");
    }

    py::list res;

    for (auto &it : func_args_) {
        res.append(it.first);
    }

    return res;
}

bool GretlPackage::is_estimator (void) const
{
    return defines_estimator_;
}

std::map<std::string, int> GretlPackage::get_func_args (const std::string &fun_name) const
{
    auto it = func_args_.find(fun_name);

    if (it != func_args_.end()) {
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
                                             const py::kwargs &kwargs)
{
    // we prepare function to call
    fncall *func_call = user_func_get_fncall(func_get_ptr(func_name));
    if (!func_call) {
        throw std::runtime_error("unable to load '" + func_name + "' function");
    }

    // we unpack parameters
    unpack_kwargs_package_(func_name, func_call, GD, kwargs);

    // we check dataset in the context of package's "data-requirement"
    if (!GD && !(get_properties_<DataReq>("data-requirement") == FN_NODATA_OK)) {
        throw std::runtime_error("No dataset is available");
    }

    // we must set returning type (must be released at the end)
    GretlPackageRes res;
    gretl_list_ptr gr_list{nullptr};

    res.set_type(func_get_res_type(func_name));

    // function execution
    err = gretl_function_exec(func_call, res.type, GD ? GD->dset : NULL,
                              res.out_ptr(), GP.prn_ptr());
    if (err) {
        handle_gretl_error(err);
    }

    // for GRETL_TYPE_SERIES we must record length
    if (res.type == GRETL_TYPE_SERIES) {
        res.n = GD->dset->n;
    }

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
