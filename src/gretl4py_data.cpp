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
#include "gretl4py.h" // FIXME: delete in GretlDataset class mapped to Python
#include <gretl/matrix_extra.h>
#include <gretl/gretl_model.h>
#include <gretl/genmain.h>
#include <gretl/objstack.h>
#include <gretl/gretl_drivers.h>
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
        throw std::invalid_argument("Element 'data_array' of given dictionary is not of NumPy array type.");
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

static std::unique_ptr<GretlTypes> convert_GretlType_to_GretlTypes (GretlType &t,
                                                                    void *gr_obj_ptr,
                                                                    const int n, int &err)
{
    //
    // function converts libgretl's GretlType to gretl4py's GretlTypes
    // *gr_obj_ptr: pointer to libgretl's object
    //
    auto res = std::make_unique<GretlTypes>();

    if (t == GRETL_TYPE_NONE) {
        std::printf("gretl_grab: didn't get anything!\n");
    } else if (t == GRETL_TYPE_DOUBLE) {
        *res = *(static_cast<double*>(gr_obj_ptr));
    } else if (t == GRETL_TYPE_STRING) {
        *res = static_cast<char*>(gr_obj_ptr);
    } else if (t == GRETL_TYPE_MATRIX) {
        gretl_matrix *gretl_mat = static_cast<gretl_matrix*>(gr_obj_ptr);
        DNumPyArrayFC mat({gretl_mat->rows, gretl_mat->cols});

        std::memcpy(static_cast<double*>(mat.request().ptr), gretl_mat->val,
                    gretl_mat->rows * gretl_mat->cols * sizeof(double));
        if ((std::min)(gretl_mat->rows, gretl_mat->cols) == 1) { // this is to prevent 'min' macro expansion on MS SDK
            mat = mat.reshape({mat.size()});
        }
        *res = mat;
    } else if (t == GRETL_TYPE_BUNDLE) {
        gretl_bundle *b = static_cast<gretl_bundle*>(gr_obj_ptr);
        *res = GretlBundle(b).to_dict();
    } else if (t == GRETL_TYPE_ARRAY) {
        gretl_array *a = static_cast<gretl_array*>(gr_obj_ptr);
        py::list py_list;
        GretlType tmp;

        for (int i=0; i<gretl_array_get_length(a); i++) {
            tmp = gretl_array_get_content_type(a);
            auto element = convert_GretlType_to_GretlTypes(tmp, gretl_array_get_element(a, i, &tmp, &err), n, err);
            py_list.append(*element);
        }
        *res = py_list;
    } else if (t == GRETL_TYPE_SERIES) {
        double *s = static_cast<double*>(gr_obj_ptr);
        DNumPyArrayFC mat(n);

        std::memcpy(static_cast<double*>(mat.request().ptr), s, n * sizeof(double));
        *res = mat;
    } else if (t == GRETL_TYPE_LIST) {
        int *l = static_cast<int*>(gr_obj_ptr);
        py::list py_list;

        for (int i=1; i<=l[0]; i++) {
            py_list.append(l[i]);
        }
        *res = py_list;
    } else {
        std::printf("gretl_grab: got unsupported type!\n");
        *res = py::none();
    }

    return res;
}


//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////

void handle_gretl_error (int err)
{
    const char *msg = errmsg_get_with_default(err);
    throw std::runtime_error(msg);
}

std::unique_ptr<GretlTypes> gretl_generate_gretl_object (const char *s, int &err,
                                                         GretlObjType obj_type,
                                                         DATASET *dset, MODEL *model)
{
    //
    // we grab object from libgretl stack
    //
    std::unique_ptr<GretlTypes> res; // gretl4py type
    GretlType t = GRETL_TYPE_NONE;   // libgretl type
    bool free_dset = false;          // we free dset _only_ if was allocated here
    void *resp;

    // we allocate dset if we didn't get one
    if (dset == NULL) {
        dset = datainfo_new();
        free_dset = true;
    }

    // we grab object from stack (note: if model was passed we use it)
    if (model != NULL) {
        set_genr_model(model, obj_type);
    }
    resp = generate_gretl_object(s, dset, &t, &err);
    if (model != NULL) {
        unset_genr_model();
    }

    if (!err) {
        res = convert_GretlType_to_GretlTypes(t, resp, dset->n, err);
    }

    if (free_dset) {
        destroy_dataset(dset);
    }

    return res;
}

void gretl_valid_long_opt (gretlopt &opt, GretlCmdIndex ci,
                           const std::string &key, const py::handle &val,
                           const char* ext_val)
{
    // should be a regular gretl option: check its validity in
    // the context of @ci
    // note: ext_val is to bypass kwargs and to force setting a given option
    gretlopt opt_local = OPT_NONE;
    OptStatus status;

    if (std::strchr(key.c_str(), '_')) {
        std::string tmp(key);

        gretl_charsub(tmp.data(), '_', '-');
        opt_local = valid_long_opt(ci, tmp.c_str(), &status);
    } else {
        opt_local = valid_long_opt(ci, key.c_str(), &status);
    }
    if (opt_local == 0) {
        throw std::runtime_error("invalid option flag: " + key);
    }

    if (status == OPT_NO_PARM) {
        // no parameter wanted, but we'll accept a boolean
        if (is_pyint(val) && !ext_val) {
            int ival = val.cast<int>();

            if (ival == 1) {
                // activate the option
                opt = static_cast<gretlopt>(opt | opt_local);
            } else if (ival != 0) {
                throw std::runtime_error("invalid option parameter");
            }
        }
    } else {
        // an option parameter is accepted, perhaps required
        if (!is_pystr(val) && !is_pyint(val) && !is_pyfloat(val) && !ext_val) {
            throw std::runtime_error("unsupported option param type");
        } else if (ext_val) {
            push_option_param(ci, static_cast<gretlopt>(opt_local), gretl_strdup(ext_val));
        } else {
            push_option_param(ci, static_cast<gretlopt>(opt_local),
                              gretl_strdup(MapPyToCpp(val).to_string().c_str()));
        }
    }
}

void unpack_kwargs_test (gretlopt &opt, const std::string &type,
                         const py::kwargs &kwargs, int &order)
{
    //
    // we setup test options
    // order: for autocorrelation or arch test (may change in future)
    //
    GretlCmdIndex ci = MODTEST;
    opt = OPT_NONE;

    // we set basic opt for given test
    if (type == "add") {
        ci = ADD;
    } else if (type == "adf") {
        ci = ADF;
    } else if (type == "arch") {
        opt = OPT_H;
    } else if (type == "autocorr") {
        opt = OPT_A;
    } else if (type == "bds") {
        ci = BDS;
    } else if (type == "bkw") {
        ci = BKW;
    } else if (type == "breusch-pagan") {
        opt = OPT_B;
    } else if (type == "chow") {
        ci = CHOW;
    } else if (type == "coeffsum") {
        ci = COEFFSUM;
    } else if (type == "coint") {
        ci = COINT;
    } else if (type == "comfac") {
        opt = OPT_C;
    } else if (type == "cusum") {
        ci = CUSUM;
    } else if (type == "difftest") {
        ci = DIFFTEST;
    } else if (type == "kpss") {
        ci = KPSS;
    } else if (type == "leverage") {
        ci = LEVERAGE;
    } else if (type == "levinlin") {
        ci = LEVINLIN;
    } else if (type == "logs") {
        opt = OPT_L;
    } else if (type == "normality") {
        opt = OPT_N;
    } else if (type == "normtest") {
        ci = NORMTEST;
    } else if (type == "omit") {
        ci = OMIT;
    } else if (type == "panel") {
        opt = OPT_P;
    } else if (type == "panspec") {
        ci = PANSPEC;
    } else if (type == "runs") {
        ci = RUNS;
    } else if (type == "qlrtest") {
        ci = QLRTEST;
    } else if (type == "reset") {
        ci = RESET;
    } else if (type == "restrict") {
        ci = RESTRICT;
    } else if (type == "squares") {
        opt = OPT_S;
    } else if (type == "white") {
        opt = OPT_W;
    } else if (type == "white-nocross") {
        opt = OPT_X;
    } else if (type == "vartest") {
        ci = VARTEST;
    } else if (type == "vif") {
        ci = VIF;
    } else if (type == "xdepend") {
        opt = OPT_D;
    } else {
        throw std::invalid_argument("unknown test");
    }

    // now we loop over kwargs;
    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::invalid_argument("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "order") {
            if (is_pyint(item.second)) {
                order = item.second.cast<int>();
                continue;
            } else {
                throw std::invalid_argument("'order' takes integers only");
            }
        } else if (key == "silent") {
            // special case: silent is processed manually later on
            continue;
        } else if (key == "plot") {
            throw std::invalid_argument("invalid option flag: " + key);
        } else {
            gretl_valid_long_opt(opt, ci, key, item.second);
        }
    }

    // we force --plot=none for tests
    // FIXME: either to preprocessor level or to dispatcher
    if (type == "cusum" || type == "leverage" || type == "qlrtest") {
        gretl_valid_long_opt(opt, ci, "plot", py::handle(), "0");
    }
}

gretl_list_ptr list_from_formula (const std::string &formula,
                                  DATASET *dset, int &err)
{
    // we create a unique_ptr for the char* with std::free as the deleter.
#if defined(_WIN32)
    std::unique_ptr<char, decltype(&std::free)> tmp(_strdup(formula.c_str()), &std::free);
#else
    std::unique_ptr<char, decltype(&std::free)> tmp(strdup(formula.c_str()), &std::free);
#endif

    // convert from a simple R-style formula to a gretl list
    gretl_charsub(tmp.get(), '~', ' ');
    gretl_charsub(tmp.get(), '+', ' ');

    // generate_list() returns a raw pointer; assign that to our unique_ptr.
    int *raw_list = generate_list(tmp.get(), dset, 0, &err);
    if (err) {
        handle_gretl_error(err);
    }

    return gretl_list_ptr(raw_list, gretl_list_deleter);
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

    id = GretlDataset::next_obj_id++;
}

GretlDataset::GretlDataset (const py::dict &pydict)
{
    //
    // alternative initializer: we fill a gretl DATASET with the content of @pydict
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
    opt = setobs_opt_from_structure(dset->structure);
    set_obs(std::to_string(pd).c_str(), dset->S[0], dset, opt);

    id = GretlDataset::next_obj_id++;
}

GretlDataset::~GretlDataset (void)
{
    if (is_default) {
        set_default_dset(nullptr);
        std::printf("default dataset was deleted\n");
    }
    GretlDataset::free_dset();
}

void GretlDataset::allocate_dset (void)
{
    GretlDataset::free_dset();
    dset = datainfo_new();
}

void GretlDataset::free_dset (void)
{
    if (dset) {
        destroy_dataset(dset);
        dset = NULL;
    }
}

py::dict GretlDataset::to_dict (void) const
{
    //
    // we convert gretl data to pydict
    //
    py::dict as_dict;
    py::list vnames;
    py::list labels;
    int i = 0;
    double *tmp = nullptr;

    as_dict[py::cast("periodicity")] = dset->pd;
    as_dict[py::cast("structure")] = dset->structure;

    // note 2024-11-20: we don't put 'const' into the python dictionary,
    // so we skip series 0 when copying from GretlDataset.dset

    // copy across the data values
    DNumPyArrayFC Z({dset->n, dset->v-1});
    tmp = static_cast<double*>(Z.request().ptr);
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

void GretlDataset::to_file (const std::string fname) const
{
    //
    // we write gretl data to file named by @fname
    // FIXME should have a second (list) argument
    //
    err = write_data(fname.c_str(), NULL, dset, OPT_NONE, NULL);
    if (err) {
        handle_gretl_error(err);
    }
}

void GretlDataset::new_series (const std::string formula)
{
    //
    // we create a series according to @formula
    //
    err = generate(formula.c_str(), dset, GRETL_TYPE_SERIES, OPT_NONE, NULL);
    if (err) {
        handle_gretl_error(err);
    }
}

py::str GretlDataset::repr (void) const
{
    py::str ret = "GretlDataset, source " + source;

    return ret;
}

void GretlDataset::print (const py::kwargs &kwargs)
{
    int summary = 0;
    int byobs = 0;
    // int *list = NULL;
    int err = 0;

    // Could restrict printing to selected series via
    // series = <whatever> in kwargs -> list.

    for (auto &item : kwargs) {
        const char *val = NULL;

        if (!is_pystr(item.first)) {
            throw std::invalid_argument("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();
        val = MapPyToCpp(item.second).to_string_view().data();

        if (key == "summary") {
            summary = std::strcmp(val, "1") == 0;
        } else if (key == "byobs") {
            byobs = std::strcmp(val, "1") == 0;
        } else if (key == "byvar") {
            if (std::strcmp(val, "1") == 0) {
                summary = byobs = 0;
            }
        } else {
            summary = byobs = 0;
            throw std::invalid_argument("print: invalid key '" + key + "'");
        }
    }

    if (summary) {
        if (dataset_is_time_series(dset)) {
            char s1[OBSLEN], s2[OBSLEN];

            ntolabel(s1, 0, dset);
            ntolabel(s2, dset->n-1, dset);
            pprintf(GP.prn_ptr(), "Time series, frequency %d, T = %d, %s to %s\n\n",
                    dset->pd, dset->n, s1, s2);
        } else if (dataset_is_panel(dset)) {
            pprintf(GP.prn_ptr(), "Panel data, N = %d, T = %d\n\n", dset->n / dset->pd,
                    dset->pd);
        } else {
            pprintf(GP.prn_ptr(), "Cross-sectional data, N = %d\n\n", dset->n);
        }
        list_series(dset, OPT_NONE, GP.prn_ptr());
    } else {
        gretlopt opt = byobs ? OPT_O : OPT_NONE;

        err = printdata(NULL, NULL, dset, opt, GP.prn_ptr());
        if (err) {
            handle_gretl_error(err);
        }
    }

    GP.print();
}

void GretlDataset::sample (const py::kwargs &kwargs)
{
    //
    // A method to mimic gretl's 'smpl' command
    //
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
            throw std::invalid_argument("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();
        val = MapPyToCpp(item.second).to_string_view().data();

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
            throw std::invalid_argument("invalid key '" + key + "' in 'sample' args");
        }
    }

    // reject invalid cases
    if (full && (t1str || t2str || opt_replace)) {
        throw std::invalid_argument("sample: invalid use of 'full'");
    } else if (restr && (t1str || t2str)) {
        throw std::invalid_argument("sample: invalid use of 'restriction'");
    } else if (opt_replace && !restr) {
        throw std::invalid_argument("sample: invalid use of 'replace'");
    }

    if (argc == 0 && optc == 0) {
        // just report the current sample
        print_smpl(dset, get_full_length_n(), OPT_NONE, GP.prn_ptr());
    } else if (argc == 0 && full) {
        // "smpl full"
        err = restore_full_sample(dset, NULL);
    } else if (restr) {
        // smpl <restr> --restrict [--replace]
        gretlopt opt = OPT_R;

        if (opt_replace) {
            opt = static_cast<gretlopt>(OPT_R | OPT_P);
        }
        err = restrict_sample(restr, NULL, dset, NULL, opt, GP.prn_ptr(), NULL);
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
        print_smpl(dset, get_full_length_n(), OPT_NONE, GP.prn_ptr());
    }

    GP.print();

    std::free(t1str);
    std::free(t2str);
    std::free(restr);
}

void GretlDataset::setobs (const py::kwargs &kwargs)
{
    //
    // A method to mimic gretl's 'setobs' command
    //
    gretlopt opt = OPT_NONE;
    char *arg1 = NULL;
    char *arg2 = NULL;
    int rcount = 0;
    int pcount = 0;
    int argc = 0;

    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::invalid_argument("option key must be a string");
        }
        const std::string &akey = item.first.cast<std::string>();
        const char *val = MapPyToCpp(item.second).to_string_view().data();

        if (akey == "periodicity") {
            arg1 = strdup(val);
            rcount++;
        } else if (akey == "unitvar") {
            arg1 = strdup(val);
            pcount++;
        } else if (akey == "startobs") {
            arg2 = strdup(val);
            rcount++;
        } else if (akey == "timevar") {
            arg2 = strdup(val);
            pcount++;
        } else if (akey == "structure") {
            if (!std::strcmp(val, "cross-section")) {
                opt = OPT_X;
            } else if (!std::strcmp(val, "time-series")) {
                opt = OPT_T;
            } else if (!std::strcmp(val, "stacked-time-series")) {
                opt = OPT_S;
            } else {
                throw std::invalid_argument("setobs: invalid 'structure' specification");
            }
        } else {
            throw std::invalid_argument("setobs: invalid key '" + akey + "'");
        }
    }

    if (argc == 0 && opt == OPT_NONE) {
        print_smpl(dset, 0, OPT_NONE, GP.prn_ptr());
        GP.print();
        return;
    }

    argc = rcount + pcount;
    if (argc < 2) {
        throw std::invalid_argument("too few arguments for 'setobs'");
    } else if (pcount > 0) {
        if (rcount > 0 || opt != OPT_NONE) {
            throw std::invalid_argument("invalid mixed modes in 'setobs'");
        } else {
            /* equiv. to --panel-vars */
            opt = OPT_P;
        }
    }

    err = set_obs(arg1, arg2, dset, opt);

    std::free(arg1);
    std::free(arg2);
}

void GretlDataset::set_as_default (void)
{
    set_default_dset(shared_from_this());
    is_default = true;
}

int GretlDataset::get_id (void) const
{
    return id;
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
    linked_models.insert(model_id);
}

void GretlDataset::linked_models_del (const int model_id)
{
    linked_models.erase(model_id);
}

std::unordered_set<int> GretlDataset::linked_models_list (void) const
{
    return linked_models;
}

gretlopt GretlDataset::setobs_opt_from_structure (int structure)
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

std::unique_ptr<GretlTypes> GretlDataset::get_accessor (const std::string &accessor) const
{
    auto res = gretl_generate_gretl_object(accessor.c_str(), err, GRETL_OBJ_DSET, dset);
    if (err) {
        handle_gretl_error(err);
    }

    return res;
}

void GretlDataset::test (const std::string &type, const std::string &vars,
                         const py::kwargs &kwargs)
{
    //
    // this method performs test which do take a variables list
    //
    py::dict res;
    int order = 0;
    gretl_list_ptr ex_var{nullptr, gretl_list_deleter};

    // we construct list of external variables
    ex_var = list_from_formula(vars, dset, err); // it's std::unique_ptr!!!
    if (err) {
        handle_gretl_error(err);
    }

    // we set up options
    unpack_kwargs_test(opt, type, kwargs, order);

    // we perform the test
    if (type == "adf") {
        err = adf_test(order, ex_var.get(), dset, opt, GP.prn_ptr());
    } else if (type == "bds") {
        err = bds_test_driver(order, ex_var.get(), dset, opt, GP.prn_ptr());
    } else if (type == "coint") {
        err = engle_granger_test(order, ex_var.get(), dset, opt, GP.prn_ptr());
    } else if (type == "difftest") {
        err = diff_test(ex_var.get(), dset, opt, GP.prn_ptr());
    } else if (type == "kpss") {
        err = kpss_test(order, ex_var.get(), dset, opt, GP.prn_ptr());
    } else if (type == "levinlin") {
        err = llc_test_driver(std::to_string(order).c_str(), ex_var.get(),
                              dset, opt, GP.prn_ptr());
    } else if (type == "normtest") {
        if (ex_var.get()[0] > 1) {
            throw std::invalid_argument("normtest takes just one series");
        }
        err = gretl_normality_test(ex_var.get()[1], dset, opt, GP.prn_ptr());
    } else if (type == "runs") {
        if (ex_var.get()[0] > 1) {
            throw std::invalid_argument("runs takes just one series");
        }
        err = runs_test(ex_var.get()[1], dset, opt, GP.prn_ptr());
    } else if (type == "vartest") {
        if (ex_var.get()[0] != 2) {
            throw std::invalid_argument("vartest takes exactly two series");
        }
        err = vars_test(ex_var.get(), dset, opt, GP.prn_ptr());
    } else {
        throw std::invalid_argument(type + " is a model-based test");
    }

    if (err) {
        handle_gretl_error(err);
    }

    res[py::cast("test")] = *get_accessor("$test");
    res[py::cast("pvalue")] = *get_accessor("$pvalue");
    res[py::cast("summary")] = GP.get_buffer();
    if (!kwargs.contains("silent")) {
        GP.print();
    }

    // we register result
    py::object self = py::cast(this);
    py::setattr(self, type.c_str(), res);
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

GretlBundle::GretlBundle (gretl_bundle *b)
{
    this->b = b;
    free_b = false;
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

py::dict GretlBundle::json_to_dict (simdjson::ondemand::object &json_obj,
                                    py::dict *outer_dict,
                                    std::string_view *outer_key,
                                    bool *add_to_outer)
{
    py::dict res;
    int mat_rows = 0;
    int mat_cols = 0;
    bool add_to_res = true;
    std::string gr_type;
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
        } else if (gr_type == "gretl_series") {
            json_obj["size"].get(GretlBundle::sd_tmp_value);
            GretlBundle::sd_tmp_number = GretlBundle::sd_tmp_value.get_number();
            mat_rows = GretlBundle::sd_tmp_number.get_int64();
            mat_cols = 1;
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

            if (gr_type == "gretl_matrix" || gr_type == "gretl_series") {
                DNumPyArrayFC mat({mat_rows, mat_cols});
                double *mat_ptr = mat.mutable_data();

                for (auto item : GretlBundle::sd_tmp_array) {
                    if (item.type() == simdjson::ondemand::json_type::string) {
                        mat_ptr[i++] = std::nan("");
                    } else {
                        mat_ptr[i++] = static_cast<double>(item);
                    }
                }

                if ((std::min)(mat_rows, mat_cols) == 1) { // this is to prevent 'min' macro expansion on MS SDK
                    (*outer_dict)[py::cast(*outer_key)] = mat.reshape({mat.size()});
                } else {
                    (*outer_dict)[py::cast(*outer_key)] = mat;
                }
                *add_to_outer = false;
            } else {
                py::list li;

                for (auto item : GretlBundle::sd_tmp_array) {
                    if (gr_type == "gretl_list" && i == 0) {
                        i = 1;
                        continue;
                    }

                    if (item.type() == simdjson::ondemand::json_type::string) {
                        item.get_string(GretlBundle::sd_tmp_str);
                        li.append(GretlBundle::sd_tmp_str);
                    } else {
                        if (gr_type == "gretl_list") {
                            li.append(static_cast<int>(item.get_int64()));
                        } else {
                            li.append(static_cast<double>(item));
                        }
                    }
                }

                if (gr_type == "gretl_list") {
                    (*outer_dict)[py::cast(*outer_key)] = li;
                    *add_to_outer = false;
                } else {
                    res[py::cast(key)] = li;
                }
            }
        } else if (val.type() == simdjson::ondemand::json_type::object) {
            val_obj = val.get_object();
            py::dict tmp_dict = GretlBundle::json_to_dict(val_obj, &res, &key, &add_to_res);

            if (add_to_res) {
                res[py::cast(key)] = tmp_dict;
            } else {
                add_to_res = true;
            }
        }
    }

    return res;
}

py::dict GretlBundle::to_dict (void)
{
    if (as_dict.empty()) {
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
        if (!GP.err) {
            err = gretl_bundle_print(b, GP.prn_ptr());
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
    this->prn_type = prn_type;

    prn = gretl_print_new(this->prn_type, &err);
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

PRN *GretlPrint::prn_ptr (void)
{
    // at first we always reset a buffer
    if (prn_type == GRETL_PRINT_BUFFER) {
        err = gretl_print_reset_buffer(prn);
        if (err) {
            handle_gretl_error(err);
        }
    }

    return prn;
}

void GretlPrint::print (void)
{
    const char *buf = gretl_print_get_buffer(prn);

    if (buf != NULL) {
        py::print(buf);
    }
}

const char *GretlPrint::get_buffer (void)
{
    return gretl_print_get_buffer(prn);
}
