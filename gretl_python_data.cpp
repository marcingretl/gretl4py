#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
//#include <pybind11/stl.h>
#include "gretl_python_data.h"
#include <gretl/matrix_extra.h>
#include <iostream>
#include <string>

#define F_CONTIGUOUS py::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_
#define is_pynum_fortranarray(m) ((m.flags() & F_CONTIGUOUS))

#define GRETL_PYBIND11_DEBUG 0

void gretl_model_to_dict (MODEL *model, DATASET *dset, py::dict &pydict)
{
    //
    // here we construct python's dict which is returned (with estimation results)
    //
    py::dict coeff, sderr, pvalue, criterion;
    py::array_t<double, py::array::f_style> vcv({model->ncoeff, model->ncoeff});
    char pname[VNAMELEN];
    int err = 0;
    gretl_matrix *m = NULL;
    std::vector<GretlCmdIndex> ZscoreCI{ARMA};
    std::vector<GretlCmdIndex>::iterator it;
    double pval;

    // we check if we deal with z-score or t-ratio based estimator
    it = std::find(ZscoreCI.begin(), ZscoreCI.end(), model->ci);
    bool zscore = (it != ZscoreCI.end() ? true : false);
    pydict[py::cast("_zscore")] = zscore;

    // dictionaries
    for (int i=0; i<model->ncoeff; i++) {
        gretl_model_get_param_name(model, dset, i, pname);
        coeff[py::cast(pname)] = model->coeff[i];
        sderr[py::cast(pname)] = model->sderr[i];
        if (zscore) {
            pval = normal_pvalue_2(std::abs(model->coeff[i]/model->sderr[i]));
        } else {
            pval = student_pvalue_2(model->dfd, std::abs(model->coeff[i]/model->sderr[i]));
        }
        pvalue[py::cast(pname)] = pval;
    }
    criterion[py::cast("aic")] = model->criterion[0];
    criterion[py::cast("bic")] = model->criterion[1];
    criterion[py::cast("hqc")] = model->criterion[2];

    pydict[py::cast("coeff")] = coeff;
    pydict[py::cast("sderr")] = sderr;
    pydict[py::cast("pvalue")] = pvalue;
    pydict[py::cast("criterion")] = criterion;

    // covariance matrix
    m = gretl_vcv_matrix_from_model(model, NULL, &err);
    if (err) {
        pydict[py::cast("vcv")] = NULL;
    } else {
        std::memcpy((double *) vcv.request().ptr, m->val, m->rows * m->cols * sizeof(double));
        pydict[py::cast("vcv")] = vcv;
    }
    gretl_matrix_free(m);

    // scalars
    pydict[py::cast("ID")] = model->ID;
    pydict[py::cast("t1")] = model->t1;
    pydict[py::cast("t2")] = model->t2;
    pydict[py::cast("nobs")] = model->nobs;
    pydict[py::cast("full_n")] = model->full_n;
    pydict[py::cast("ncoeff")] = model->ncoeff;
    pydict[py::cast("dfn")] = model->dfn;
    pydict[py::cast("dfd")] = model->dfd;
    pydict[py::cast("ess")] = model->ess;
    pydict[py::cast("tss")] = model->tss;
    pydict[py::cast("sigma")] = model->sigma;
    pydict[py::cast("rsq")] = model->rsq;
    pydict[py::cast("adjrsq")] = model->adjrsq;
    pydict[py::cast("fstt")] = model->fstt;
    pydict[py::cast("chisq")] = model->chisq;
    pydict[py::cast("lnL")] = model->lnL;
    pydict[py::cast("ybar")] = model->ybar;
    pydict[py::cast("sdy")] = model->sdy;
    pydict[py::cast("dw")] = model->dw;
    pydict[py::cast("rho")] = model->rho;
    pydict[py::cast("nparams")] = model->nparams;
    pydict[py::cast("esttime")] = model->esttime;
}

static void copy_string_from_python_dict (const py::dict &pydict,
                                          const char *key, char *dest,
                                          int n_bytes, int offset = 0)
{
    //
    // here we take just one element (under the "key") and copy to dest
    //
    std::string stdBUF;
    auto element = pydict[py::cast(key)]; //pydict[py::cast(key)] is of py::str type

    if (py::isinstance<py::int_>(element)) {
        stdBUF = std::to_string(py::cast<int>(element));
    } else {
        stdBUF = py::cast<std::string>(element);
    }

    stdBUF.erase(std::remove_if(stdBUF.begin(), stdBUF.end(), ::isspace), stdBUF.end());
    stdBUF.copy(dest, n_bytes, offset);
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
    std::string stdBUF;
    int i = dst_offset;

    for (auto item : pylist) {
        stdBUF = std::string(py::str(item));
        if (trim) {
            stdBUF.erase(std::remove_if(stdBUF.begin(), stdBUF.end(), ::isspace), stdBUF.end());
        }
        stdBUF.copy(dest[i++], n_bytes, src_offset);
    }
}

static void copy_strings_from_python_list (const py::list &pylist,
                                           std::string *strBUF, bool trim = false)
{
    //
    // we copy the whole list to strBUF (length must be verified at the caller level)
    // with optional clearing
    //
    int i = 0;
    std::string str;

    for (auto item : pylist) {
        str = std::string(py::str(item));
        if (trim) {
            str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
        }
        strBUF[i++] = str;
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

// two overloaded
void numpy_array_to_gretl_storage (const py::array_t<double> &pynum_array,
                                   gretl_matrix *m)
{
    // we copy pynum_array to empty m
    if (m->rows || m->cols) {
        throw std::runtime_error("Given matrix is not empty.");
    }

    ssize_t rows, cols;
    int fortranarray;
    double *data_array = NULL;

    data_array = numpy_array_extract(pynum_array, &rows, &cols, &fortranarray);
    if (data_array == NULL) {
        throw std::runtime_error("Numpy array: null pointer.");
    }

    if (gretl_matrix_realloc(m, rows, cols)) {
        throw std::runtime_error("Matrix allocation failed.");
    }

    if (fortranarray) {
        std::memcpy(m->val, data_array, rows * cols * sizeof(double));
    } else {
        for (int i=0; i<rows; i++) {
            for (int j=0; j<cols; j++) {
                gretl_matrix_set(m, i, j, *(data_array+i*cols+j));
            }
        }
    }
}

static void numpy_array_to_gretl_storage (const py::dict &pydict,
                                          DATASET *dset)
{
    // we copy "data_array" which lives in pydict to dset->Z
    ssize_t rows, cols;
    int fortranarray;
    double *data_array = NULL;

    // the old code: do not delete for a while
    //py::array_t<double> pynum_array = py::cast<py::array>(pydict[py::cast("data_array")]);
    //py::array_t<double> *pynum_array_new = static_cast<py::array_t<double>*> (py::cast<py::array>(pydict[py::cast("data_array")]).mutable_data());
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
    for (int i=0; i<dset->v-1; i++) {
        if (fortranarray) {
            std::memcpy(dset->Z[i+1], data_array+i*rows, rows * sizeof(double));
        } else {
            for (int j=0; j<rows; j++) {
                *(dset->Z[i+1]+j) = *(data_array+i+j*cols);
            }
        }
    }
    dset->t1 = 0;
    dset->t2 = dset->n-1;
}

void numpy_dict_to_gretl_dset (const py::dict &pydict,
                               DATASET *dset)
{
    //
    // we fill dset with pydict content
    //
    py::list pylist;
    std::string *strBUF;
    char periodicity[16];

    if (dset == NULL) {
        throw std::runtime_error("Dataset allocation failed.");
    }

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

    // FIXME: maybe it's possible automatically, i.e. without sending it from Python
    copy_string_from_python_dict(pydict, "periodicity", periodicity, 16);
    set_obs(periodicity, dset->S[0], dset, OPT_T);
}

void gretl_dset_to_numpy_dict (char *fname, py::dict &pydict)
{
    py::list *vnames = new py::list;
    py::list *labels = new py::list;
    DATASET *dset;
    double *tmp;
    int i, err = 0;

    dset = datainfo_new();
    err = gretl_get_data(fname, dset, OPT_NONE, NULL);
    if (err) {
        delete vnames;
        delete labels;
        destroy_dataset(dset);
        throw std::runtime_error("Reading data from gdt file failed.");
    }

    // copy across the data values
    py::array_t<double, py::array::f_style> Z({dset->n, dset->v});
    tmp = (double *) Z.request().ptr;
    for (i=0; i<dset->v; i++) {
        std::memcpy(tmp, dset->Z[i], dset->n * sizeof *tmp);
        tmp += dset->n;
    }
    pydict[py::cast("data_array")] = Z;

    // copy across the series names
    for (i=0; i<dset->v; i++) {
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
        for (i=0; i<dset->n; i++) {
            labels->append(std::to_string(i+1));
        }
    }
    pydict[py::cast("obs_labels")] = *labels;
    delete labels;

    // copy dataset periodicity
    pydict[py::cast("periodicity")] = dset->pd;

    destroy_dataset(dset);
}
