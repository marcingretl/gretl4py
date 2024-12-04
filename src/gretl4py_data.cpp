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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "gretl4py_data.h"
#include <gretl/matrix_extra.h>
#include <gretl/gretl_model.h>
#include <iostream>
#include <string>

#define F_CONTIGUOUS py::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_
#define is_pynum_fortranarray(m) ((m.flags() & F_CONTIGUOUS))

#define GRETL_PYBIND11_DEBUG 0

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

// two overloaded
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

static double *numpy_array_extract (const py::array_t<double> &pynum_array,
                                    ssize_t *rows, ssize_t *cols, int *fortranarray)
{
    //
    // we check dimensions and if OK we extract most important 'pynum_array' metadata
    //
    if (pynum_array.ndim() > 2) {
        throw std::runtime_error("Given array has more than two dimensions.");
    } else if (pynum_array.ndim() == 1) {
        *cols = 1;
    } else {
        *cols = pynum_array.shape(1);
    }
    *rows = pynum_array.shape(0);
    *fortranarray = is_pynum_fortranarray(pynum_array);

    return (double*) pynum_array.request().ptr;
}

static void numpy_array_to_gretl_storage (const py::dict &pydict,
                                          DATASET *dset)
{
    //
    // we copy "data_array" which lives in pydict to dset->Z
    //
    // we check if pydict["data_array"] is of <py::array_t> type
    if (!py::isinstance<py::array_t<double>>(pydict[py::cast("data_array")])) {
        throw std::runtime_error("Element 'data_array' of given dictionary is not of NumPy array type.");
    }

    ssize_t rows, cols, colsize;
    int fortranarray;
    double *data_array = NULL;
    int k;

    data_array = numpy_array_extract(py::cast<py::array>(pydict[py::cast("data_array")]),
                                     &rows, &cols, &fortranarray);
    if (data_array == NULL) {
        throw std::runtime_error("Numpy array: null pointer.");
    }

    dset->n = rows;
    dset->v = cols + 1;
    if (allocate_Z(dset, OPT_NONE)) {
        throw std::runtime_error("Matrix allocation failed.");
    }

    colsize = rows * sizeof(double);
    for (int i=1; i<dset->v; i++) {
        k = i - 1;
        if (fortranarray) {
            std::memcpy(dset->Z[i], data_array + k*rows, colsize);
        } else {
            for (int j=0; j<rows; j++) {
                dset->Z[i][j] = *(data_array + k + j*cols);
            }
        }
    }
    dset->t1 = 0;
    dset->t2 = dset->n-1;
}

//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////

void finalize_buffered_printer (PRN *prn)
{
    const char *buf = gretl_print_get_buffer(prn);

    if (buf != NULL) {
        py::print(buf);
    }
    gretl_print_destroy(prn);
}

PRN *get_buffered_printer (void)
{
    PRN *prn = nullptr;
    int err = 0;

    prn = gretl_print_new(GRETL_PRINT_BUFFER, &err);
    if (err) {
        handle_gretl_error(err);
    }

    return prn;
}

void handle_gretl_error (int err)
{
    const char *msg = errmsg_get_with_default(err);
    throw std::runtime_error(msg);
}

int gretl_model_to_dict (MODEL *model, DATASET *dset,
                         const char *summary, py::dict &pydict)
{
    //
    // here we construct a python dictionary holding estimation results
    //
    GretlBundle b = GretlBundle(model, dset);
    char *json_buf = b.to_json();

    if (!b.myerr) {
        pydict[py::cast("_id")] = model->ID;
        pydict[py::cast("json_buffer")] = json_buf;
        pydict[py::cast("summary")] = summary;
    }

    return b.myerr;
}


//////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////

//
//GretlDataset class methods
//
GretlDataset::GretlDataset (const std::string fname)
{
    //
    // load gretl data from file
    //
    int err;

    allocate_dset();
    if (dset) {
        err = gretl_get_data(fname.c_str(), dset, OPT_NONE, NULL);
    } else {
        err = E_ALLOC;
    }
    if (err) {
        destroy_dataset(dset);
        throw std::runtime_error("Reading data from '" + fname + "' file failed.");
    }
}

static gretlopt setobs_opt_from_structure (int structure)
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

GretlDataset::GretlDataset (const py::dict &pydict)
{
    //
    // we fill a gretl DATASET struct with the content of @pydict
    //
    py::list pylist;
    std::string *strBUF;
    int pd;
    gretlopt opt = OPT_NONE;

    allocate_dset();

    // we fill up dset->Z
    numpy_array_to_gretl_storage(pydict, dset);

    if (dataset_allocate_varnames(dset) || dataset_allocate_obs_markers(dset)) {
        throw std::runtime_error("Dataset strings allocation failed.");
    }

    // we fill up dset->S
    pylist = pydict[py::cast("obs_labels")];
    if (pylist.size() != (long unsigned int) dset->n) {
        throw std::runtime_error("Number of obs_labels is different than number of observations.");
    }
    copy_strings_from_python_list(pylist, dset->S, OBSLEN-1, 0, 0, true);

    // we fill up dset->varname
    pylist = pydict[py::cast("varname")];
    if (pylist.size() != (long unsigned int) dset->v-1) {
        throw std::runtime_error("Number of varnames is different than number of variables.");
    }
    strBUF = new std::string[dset->v];
    copy_strings_from_python_list(pylist, strBUF);
    for (int i=0; i<dset->v-1; i++) {
        std::memset(dset->varname[i+1], '\0', VNAMELEN);
        gretl_normalize_varname(dset->varname[i+1], strBUF[i].c_str(), 1, 0);
    }
    delete []strBUF;

    dset->structure = get_int_from_python_dict(pydict, "structure", 0);

    // FIXME: maybe it's possible automatically, i.e. without sending it from Python
    pd = get_int_from_python_dict(pydict, "pd");
    opt = setobs_opt_from_structure(dset->structure);
    set_obs(std::to_string(pd).c_str(), dset->S[0], dset, opt);
}

GretlDataset::~GretlDataset (void)
{
    free_dset();
}

py::dict *GretlDataset::to_dict (void)
{
    //
    // we convert gretl data to pydict
    //
    py::dict *as_dict = new py::dict;
    py::list *vnames = new py::list;
    py::list *labels = new py::list;
    auto &pydict = *as_dict;
    double *tmp;
    int i = 0;

    pydict[py::cast("pd")] = dset->pd;
    pydict[py::cast("structure")] = dset->structure;

    // note 2024-11-20: we don't put 'const' into the python dictionary,
    // so we skip series 0 when copying from GretlDataset.dset

    // copy across the data values
    py::array_t<double, py::array::f_style> Z({dset->n, dset->v-1});
    tmp = (double *) Z.request().ptr;
    for (i=1; i<dset->v; i++) {
        std::memcpy(tmp, dset->Z[i], dset->n * sizeof *tmp);
        tmp += dset->n;
    }
    pydict[py::cast("data_array")] = Z;

    // copy across the series names
    for (i=1; i<dset->v; i++) {
        vnames->append(dset->varname[i]);
    }
    pydict[py::cast("varname")] = *vnames;
    delete vnames;

    // copy or manufacture obs labels
    if (dset->S != NULL) {
        for (i=0; i<dset->n; i++) {
            labels->append(dset->S[i]);
        }
    } else {
        char s[OBSLEN];

        for (i=0; i<dset->n; i++) {
            labels->append(ntolabel(s, i, dset));
        }
    }
    pydict[py::cast("obs_labels")] = *labels;
    delete labels;

    return as_dict;
}

void GretlDataset::printdset (bool namesonly)
{
    PRN *prn;

    prn = get_buffered_printer();

    if (namesonly) {
        list_series(dset, OPT_NONE, prn);
    } else {
        int err = printdata(NULL, NULL, dset, OPT_O, prn);

        if (err) {
            handle_gretl_error(err);
        }
    }

    finalize_buffered_printer(prn);
}

void GretlDataset::sample (const py::kwargs &kwargs)
{
    //
    // method which mimics gretl's 'smpl' command
    //
    char *t1str = nullptr;
    char *t2str = nullptr;
    char *restr = nullptr;
    PRN *prn = nullptr;
    const char *val;
    int opt_replace = 0;
    int full = 0;
    int argc = 0;
    int optc = 0;
    int err = 0;

    for (auto &item : kwargs) {
        val = NULL;

        if (!is_pystr(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (is_pyint(item.second) || is_pybool(item.second)) {
            val = std::to_string(item.second.cast<int>()).c_str();
        } else if (is_pystr(item.second)) {
            val = item.second.cast<std::string_view>().data();
        }
        if (key == "t1") {
            t1str = gretl_strdup(val);
            argc++;
        } else if (key == "t2") {
            t2str = gretl_strdup(val);
            argc++;
        } else if (key == "restriction") {
            restr = gretl_strdup(val);
            argc++;
        } else if (key == "replace") {
            opt_replace = std::atoi(val);
            optc++;
        } else if (key == "full") {
            full = std::atoi(val);
            optc++;
        } else {
            throw std::runtime_error("invalid key '" + key + "' in 'sample' args");
        }
    }

    // reject invalid cases
    if (full && (t1str || t2str || opt_replace)) {
        throw std::runtime_error("invalid use of 'full' in 'sample' args");
    } else if (restr && (t1str || t2str)) {
        throw std::runtime_error("invalid use of 'restriction' in 'sample' args");
    } else if (opt_replace && !restr) {
        throw std::runtime_error("invalid use of 'replace' in 'sample' args");
    }

    prn = get_buffered_printer();

    if (argc == 0 && optc == 0) {
        // just report the current sample
        print_smpl(dset, get_full_length_n(), OPT_NONE, prn);
    } else if (argc == 0 && full) {
        // "smpl full"
        err = restore_full_sample(dset, NULL);
    } else if (restr) {
        // smpl <restr> --restrict [--replace]
        gretlopt opt = OPT_R;

        if (opt_replace) {
            // C++ !!
            opt = (gretlopt) ((int) OPT_R | (int) OPT_P);
        }
        err = restrict_sample(restr, NULL, dset, NULL, opt, prn, NULL);
    } else if (t1str || t2str) {
        // smpl <t1str> <t2str>
        if (t1str && !t2str) {
            err = set_sample(t1str, ";", dset, OPT_NONE);
        } else if (t2str && !t1str) {
            err = set_sample(";", t2str, dset, OPT_NONE);
        } else {
            err = set_sample(t1str, t2str, dset, OPT_NONE);
        }
    }

    if (err) {
        const char *msg = errmsg_get_with_default(err);
        throw std::runtime_error(msg);
    } else if (argc > 0 || optc > 0) {
        print_smpl(dset, get_full_length_n(), OPT_NONE, prn);
    }

    finalize_buffered_printer(prn);

    std::free(t1str);
    std::free(t2str);
    std::free(restr);
}

//
// GretlBundle class methods
//
GretlBundle::GretlBundle (void)
{
    b = get_sysinfo_bundle(&myerr);
    free_b = false;
}

GretlBundle::GretlBundle (MODEL *model, DATASET *dset)
{
    b = bundle_from_model(model, dset, &myerr);
}

GretlBundle::~GretlBundle (void)
{
    if (json_buf) {
        g_free(json_buf);
    }
    if (free_b && b) {
        gretl_bundle_destroy(b);
    }
}

char *GretlBundle::to_json (void)
{
    if (!myerr && !json_buf) {
        json_buf = gretl_bundle_write_to_json_buffer(b, &myerr);
    }

    return json_buf;
}

void GretlBundle::print (void)
{
    if (!myerr) {
        PRN *prn = get_buffered_printer();

        if (prn) {
            gretl_bundle_print(b, prn);
            finalize_buffered_printer(prn);
        }
    }
}

//
// MapToPy class methods
//
MapToPy::MapToPy (const std::optional<py::object> &pyobject)
{
#if GRETL_PYBIND11_DEBUG
    std::printf("converting %s to string\n",
                py::cast<std::string>(pyobject->get_type()).c_str());
# endif
    res = &*pyobject;

    if (is_pystr(*pyobject)) {
        as_str = py::cast<std::string>(*pyobject);
    } else if (is_pyint(*pyobject)) {
        as_str = std::to_string(py::cast<int>(*pyobject));
    } else {
        as_str.clear();
        std::cerr << "I don't know how to convert '";
        std::cerr << py::cast<std::string>(pyobject->get_type());
        std::cerr << "' to std::string" << std::endl;
    }
}
