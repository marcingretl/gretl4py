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
#include <gretl/matrix_extra.h>
#include <gretl/gretl_model.h>
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

static double *numpy_array_extract (const py::array_t<double> &pynum_array,
                                    int *rows, int *cols, int *fortranarray)
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

    return static_cast<double*>(pynum_array.request().ptr);
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

    double *data_array = nullptr;
    int rows, cols;
    size_t colsize;
    int fortranarray;
    int k;

    data_array = numpy_array_extract(py::cast<py::array>(pydict[py::cast("data_array")]),
                                     &rows, &cols, &fortranarray);
    if (!data_array) {
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

//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////

void handle_gretl_error (int err)
{
    const char *msg = errmsg_get_with_default(err);
    throw std::runtime_error(msg);
}

py::dict *gretl_model_to_dict (MODEL *model, DATASET *dset,
                               const char *summary, int *err)
{
    //
    // Here we construct a python dictionary which is
    // the representation of the gretl_bundle produced by
    // libgretl's bundle_from_model(). We also add the
    // @summary text produced by libgretl's printmodel();

    // and a string holding the JSON representation of the gretl_bundle
    // produced by libgretl's bundle_from_model().
    //
    py::dict *pydict = nullptr;

    GretlBundle b = GretlBundle(model, dset);
    *err = b.err;

    if (!*err) {
        pydict = b.to_dict();

        if (!pydict) {
            *err = E_DATA;
            throw std::runtime_error("Model bundle to dict conversion failed.");
        } else {
            py::dict &d = *pydict;
            d[py::cast("_id")] = model->ID;
            d[py::cast("summary")] = summary;
        }
    }

    return pydict;
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

    GretlDataset::allocate_dset();
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

GretlDataset::GretlDataset (const py::dict &pydict)
{
    //
    // we fill a gretl DATASET struct with the content of @pydict
    //
    gretlopt opt = OPT_NONE;
    py::list pylist;
    std::string *strBUF;
    int pd;

    GretlDataset::allocate_dset();

    // we fill up dset->Z
    numpy_array_to_gretl_storage(pydict, dset);

    if (dataset_allocate_varnames(dset) || dataset_allocate_obs_markers(dset)) {
        throw std::runtime_error("Dataset strings allocation failed.");
    }

    // we fill up dset->S
    pylist = pydict[py::cast("obs_labels")];
    if (pylist.size() != static_cast<long unsigned int>(dset->n)) {
        throw std::runtime_error("Number of obs_labels is different than number of observations.");
    }
    copy_strings_from_python_list(pylist, dset->S, OBSLEN-1, 0, 0, true);

    // we fill up dset->varname
    pylist = pydict[py::cast("varname")];
    if (pylist.size() != static_cast<long unsigned int>(dset->v-1)) {
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
    GretlDataset::free_dset();
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
    int i = 0;
    double *tmp = nullptr;

    pydict[py::cast("pd")] = dset->pd;
    pydict[py::cast("structure")] = dset->structure;

    // note 2024-11-20: we don't put 'const' into the python dictionary,
    // so we skip series 0 when copying from GretlDataset.dset

    // copy across the data values
    py::array_t<double, py::array::f_style> Z({dset->n, dset->v-1});
    tmp = static_cast<double*>(Z.request().ptr);
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
    GretlPrint GP = GretlPrint();

    if (namesonly) {
        list_series(dset, OPT_NONE, GP.prn);
    } else {
        err = printdata(NULL, NULL, dset, OPT_O, GP.prn);
        if (err) {
            handle_gretl_error(err);
        }
    }

    GP.print();
}

void GretlDataset::sample (const py::kwargs &kwargs)
{
    //
    // method which mimics gretl's 'smpl' command
    //
    GretlPrint GP = GretlPrint();
    int opt_replace = 0;
    int full = 0;
    int argc = 0;
    int optc = 0;
    char *t1str = NULL;
    char *t2str = NULL;
    char *restr = NULL;
    const char *val;

    for (auto &item : kwargs) {
        val = NULL;

        if (!is_pystr(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

#if 1
        val = MapPyToCpp(item.second).to_string_view().data();
#else
        if (is_pyint(item.second) || is_pybool(item.second)) {
            val = std::to_string(item.second.cast<int>()).c_str();
        } else if (is_pystr(item.second)) {
            val = item.second.cast<std::string_view>().data();
        }
#endif // 1

        if (key == "t1") {
            t1str = strdup(val);
            argc++;
        } else if (key == "t2") {
            t2str = strdup(val);
            argc++;
        } else if (key == "restriction") {
            restr = strdup(val);
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

    if (argc == 0 && optc == 0) {
        // just report the current sample
        print_smpl(dset, get_full_length_n(), OPT_NONE, GP.prn);
    } else if (argc == 0 && full) {
        // "smpl full"
        err = restore_full_sample(dset, NULL);
    } else if (restr) {
        // smpl <restr> --restrict [--replace]
        gretlopt opt = OPT_R;

        if (opt_replace) {
            // C++ !!
            opt = static_cast<gretlopt>(static_cast<int>(OPT_R) | static_cast<int>(OPT_P));
        }
        err = restrict_sample(restr, NULL, dset, NULL, opt, GP.prn, NULL);
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
        print_smpl(dset, get_full_length_n(), OPT_NONE, GP.prn);
    }

    GP.print();

    std::free(t1str);
    std::free(t2str);
    std::free(restr);
}

//
// GretlBundle class methods
//
GretlBundle::GretlBundle (void)
{
    b = get_sysinfo_bundle(&err);
    free_b = false;
}

GretlBundle::GretlBundle (MODEL *model, DATASET *dset)
{
    b = bundle_from_model(model, dset, &err);
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

py::dict *GretlBundle::json_to_dict (simdjson::ondemand::object &json_obj,
                                     py::dict *outer_dict,
                                     std::string_view *outer_key,
                                     bool *add_to_outer)
{
    py::dict *RES = new py::dict;
    py::dict &res = *RES;
    int mat_rows = 0;
    int mat_cols = 0;
    int shift = 0; // this is because in case of gretl_list the first element just holds the list size
    bool add_to_res = true;
    std::string gr_type; // we introduce fake "gretl_vector" type which we use when gretl_matrix is in fact vector
    std::string_view key;
    simdjson::ondemand::value val;
    simdjson::ondemand::object val_obj;

    if (!json_obj["type"].get_string(gr_type)) {
        if (gr_type == "gretl_matrix") {
            json_obj["rows"].get(GretlBundle::sd_tmp_value);
            GretlBundle::sd_tmp_number = GretlBundle::sd_tmp_value.get_number();
            mat_rows = GretlBundle::sd_tmp_number.get_int64();
            json_obj["cols"].get(GretlBundle::sd_tmp_value);
            GretlBundle::sd_tmp_number = GretlBundle::sd_tmp_value.get_number();
            mat_cols = GretlBundle::sd_tmp_number.get_int64();
            if ((std::min)(mat_rows, mat_cols) == 1) { // this is to prevent 'min' macro expansion on MS SDK
                gr_type = "gretl_vector";
            }
        } else if (gr_type == "gretl_list") {
            shift = 1;
        }
    }
    json_obj.reset();

    // we iterate over all keys in json
    // note: if val is an object, we use recursion
    for (auto field : json_obj) {
        key = field.escaped_key();
        val = field.value();

        if (key == "type" || key == "rows" || key == "cols" || key == "size") {
            continue;
        }

        if (val.type() == simdjson::ondemand::json_type::null) {
            res[py::cast(key)] = py::none();
        } else if (val.type() == simdjson::ondemand::json_type::number) {
            res[py::cast(key)] = static_cast<double>(val);
        } else if (val.type() == simdjson::ondemand::json_type::string) {
            val.get_string(GretlBundle::sd_tmp_str);
            res[py::cast(key)] = GretlBundle::sd_tmp_str;
        } else if (val.type() == simdjson::ondemand::json_type::array) {
            GretlBundle::sd_tmp_array = val.get_array();
            int i = 0;

            if (gr_type == "gretl_matrix") {
                vgretl_matrix vec(mat_rows, vgretl_vector(mat_cols));
                int j = 0;

                for (auto item : GretlBundle::sd_tmp_array) {
                    if (item.type() == simdjson::ondemand::json_type::string) {
                        item.get_string(GretlBundle::sd_tmp_str);
                        vec[i][j] = GretlBundle::sd_tmp_str;
                    } else {
                        vec[i][j] = static_cast<double>(item);
                    }

                    if (++i == mat_rows) {
                        i = 0;
                        j++;
                    }
                }

                py::dict &res_outer_dict = *outer_dict;
                res_outer_dict[py::cast(*outer_key)] = vec;
                *add_to_outer = false;
            } else {
                int tmp_array_size = GretlBundle::sd_tmp_array.count_elements();
                vgretl_vector vec(tmp_array_size-shift);

                for (auto item : GretlBundle::sd_tmp_array) {
                    if (gr_type == "gretl_list" && i == 0) {
                        i++;
                        continue;
                    }

                    if (item.type() == simdjson::ondemand::json_type::string) {
                        item.get_string(GretlBundle::sd_tmp_str);
                        vec[i-shift] = GretlBundle::sd_tmp_str;
                    } else {
                        if (gr_type == "gretl_list") {
                            vec[i-shift] = static_cast<int>(item.get_int64());
                        } else {
                            vec[i-shift] = static_cast<double>(item);
                        }
                    }
                    i++;
                }

                if (gr_type == "gretl_series" || gr_type == "gretl_list" || gr_type == "gretl_vector") {
                    py::dict &res_outer_dict = *outer_dict;
                    res_outer_dict[py::cast(*outer_key)] = vec;
                    *add_to_outer = false;
                } else {
                    res[py::cast(key)] = vec;
                }
            }
        } else if (val.type() == simdjson::ondemand::json_type::object) {
            val_obj = val.get_object();
            py::dict *tmp_dict = GretlBundle::json_to_dict(val_obj, RES, &key, &add_to_res);

            if (add_to_res) {
                res[py::cast(key)] = *tmp_dict;
            } else {
                add_to_res = true;
            }
        }
    }

    return RES;
}

py::dict *GretlBundle::to_dict (void)
{
    if (!as_dict) {
        if (!err && !json_buf) {
            json_buf = gretl_bundle_write_to_json_buffer(b, &err);
        }

        if (json_buf != NULL) {
            GretlBundle::sd_json_cp = json_buf;
            if (GretlBundle::sd_parser.iterate(GretlBundle::sd_json_cp).get(GretlBundle::sd_doc)) {
                err = E_DATA;
            } else {
                GretlBundle::sd_obj = GretlBundle::sd_doc.get_object();
                as_dict = GretlBundle::json_to_dict(GretlBundle::sd_obj);
            }
        }
    }

    return as_dict;
}

void GretlBundle::print (void)
{
    if (!err) {
        GretlPrint GP = GretlPrint();

        if (!GP.err) {
            err = gretl_bundle_print(b, GP.prn);
            if (!err) {
                GP.print();
            }
        }
    }
}

//
// MapPyToCpp class methods
//
MapPyToCpp::MapPyToCpp (const std::optional<py::object> &pyobject)
{
    MapPyToCpp::convert<py::object>(&*pyobject);
}

MapPyToCpp::MapPyToCpp (const py::handle &pyhandle)
{
    MapPyToCpp::convert<py::handle>(&pyhandle);
}

template<typename T>
void MapPyToCpp::convert (const T *pyobj)
{
#if GRETL_PYBIND11_DEBUG
    std::printf("converting %s to string\n",
                py::cast<std::string>(pyobj->get_type()).c_str());
# endif
    if (is_pystr(*pyobj)) {
        as_str = py::cast<std::string>(*pyobj);
    } else if (is_pyint(*pyobj) || is_pybool(*pyobj)) {
        as_str = std::to_string(py::cast<int>(*pyobj));
    } else if (is_pyfloat(*pyobj)) {
        as_str = std::to_string(py::cast<double>(*pyobj));
    } else {
        err = E_TYPES;
        as_str.clear();
        std::cerr << "I don't know how to convert '";
        std::cerr << py::cast<std::string>(pyobj->get_type());
        std::cerr << "' to std::string" << std::endl;
    }
}

//
// GretlPrint class methods
//
GretlPrint::GretlPrint (PrnType prn_type)
{
    prn = gretl_print_new(prn_type, &err);
    if (err) {
        handle_gretl_error(err);
    }
}

GretlPrint::~GretlPrint (void)
{
    if (prn != NULL) {
        gretl_print_destroy(prn);
    }
}

void GretlPrint::print (void)
{
    const char *buf = gretl_print_get_buffer(prn);

    if (buf != NULL) {
        py::print(buf);
    }
}
