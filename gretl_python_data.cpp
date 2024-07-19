#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
//#include <pybind11/stl.h>
#include "gretl_python_data.h"
#include <gretl/matrix_extra.h>
#include <iostream>
#include <string>

#define F_CONTIGUOUS pybind11::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_
#define is_pynum_fortranarray(m) ((m.flags() & F_CONTIGUOUS))

#define GRETL_PYBIND11_DEBUG 0

void gretl_model_to_dict (MODEL *model, DATASET *dset, pybind11::dict &pydict)
{
    //
    // here we construct python's dict which is returned (with estimation results)
    //
    pybind11::dict coeff, sderr, pvalue, criterion;
    pybind11::array_t<double, pybind11::array::f_style> vcv({model->ncoeff, model->ncoeff});
    char pname[VNAMELEN];
    int err = 0;
    gretl_matrix *m = NULL;

    // dictionaries
    for (int i=0; i<model->ncoeff; i++) {
	gretl_model_get_param_name(model, dset, i, pname);
        coeff[pybind11::cast(pname)] = model->coeff[i];
        sderr[pybind11::cast(pname)] = model->sderr[i];
        pvalue[pybind11::cast(pname)] = normal_pvalue_2(std::abs(model->coeff[i]/model->sderr[i]));
    }
    criterion[pybind11::cast("aic")] = model->criterion[0];
    criterion[pybind11::cast("bic")] = model->criterion[1];
    criterion[pybind11::cast("hqc")] = model->criterion[2];

    pydict[pybind11::cast("coeff")] = coeff;
    pydict[pybind11::cast("sderr")] = sderr;
    pydict[pybind11::cast("pvalue")] = pvalue;
    pydict[pybind11::cast("criterion")] = criterion;

    // covariance matrix
    m = gretl_vcv_matrix_from_model(model, NULL, &err);
    if (err) {
        pydict[pybind11::cast("vcv")] = NULL;
    } else {
        std::memcpy((double *) vcv.request().ptr, m->val, m->rows * m->cols * sizeof(double));
        pydict[pybind11::cast("vcv")] = vcv;
    }
    gretl_matrix_free(m);

    // scalars
    pydict[pybind11::cast("ID")] = model->ID;
    pydict[pybind11::cast("t1")] = model->t1;
    pydict[pybind11::cast("t2")] = model->t2;
    pydict[pybind11::cast("nobs")] = model->nobs;
    pydict[pybind11::cast("full_n")] = model->full_n;
    pydict[pybind11::cast("ncoeff")] = model->ncoeff;
    pydict[pybind11::cast("dfn")] = model->dfn;
    pydict[pybind11::cast("dfd")] = model->dfd;
    pydict[pybind11::cast("ess")] = model->ess;
    pydict[pybind11::cast("tss")] = model->tss;
    pydict[pybind11::cast("sigma")] = model->sigma;
    pydict[pybind11::cast("rsq")] = model->rsq;
    pydict[pybind11::cast("adjrsq")] = model->adjrsq;
    pydict[pybind11::cast("fstt")] = model->fstt;
    pydict[pybind11::cast("chisq")] = model->chisq;
    pydict[pybind11::cast("lnL")] = model->lnL;
    pydict[pybind11::cast("ybar")] = model->ybar;
    pydict[pybind11::cast("sdy")] = model->sdy;
    pydict[pybind11::cast("dw")] = model->dw;
    pydict[pybind11::cast("rho")] = model->rho;
    pydict[pybind11::cast("nparams")] = model->nparams;
    pydict[pybind11::cast("esttime")] = model->esttime;
}

static void copy_string_from_python_dict (const pybind11::dict &pydict,
                                          const char *key, char *dest,
                                          int n_bytes, int offset = 0)
{
    //
    // here we take just one element (under the "key") and copy to dest
    //
    std::string stdBUF;
    auto element = pydict[pybind11::cast(key)]; //pydict[pybind11::cast(key)] is of pybind11::str type

    if (pybind11::isinstance<pybind11::int_>(element)) {
        stdBUF = std::to_string(pybind11::cast<int>(element));
    } else {
        stdBUF = pybind11::cast<std::string>(element);
    }

    stdBUF.erase(std::remove_if(stdBUF.begin(), stdBUF.end(), ::isspace), stdBUF.end());
    stdBUF.copy(dest, n_bytes, offset);
}

// two overloaded
static void copy_strings_from_python_list (const pybind11::list &pylist,
                                           char **dest, int n_bytes, int src_offset = 0,
                                           int dst_offset = 0, bool trim = false)
{
    //
    // we copy the whole list to dest (length must be verified at the caller level)
    // with optional clearing
    //
    std::string stdBUF;
    int i = dst_offset;

    for (auto item : pylist) {
        stdBUF = std::string(pybind11::str(item));
        if (trim) {
            stdBUF.erase(std::remove_if(stdBUF.begin(), stdBUF.end(), ::isspace), stdBUF.end());
        }
        stdBUF.copy(dest[i++], n_bytes, src_offset);
    }
}

static void copy_strings_from_python_list (const pybind11::list &pylist,
                                           std::string *strBUF, bool trim = false)
{
    //
    // we copy the whole list to strBUF (length must be verified at the caller level)
    // with optional clearing
    //
    int i = 0;
    std::string str;

    for (auto item : pylist) {
        str = std::string(pybind11::str(item));
        if (trim) {
            str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
        }
        strBUF[i++] = str;
    }
}

static double *numpy_array_extract (const pybind11::array_t<double> &pynum_array,
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
void numpy_array_to_gretl_storage (const pybind11::array_t<double> &pynum_array,
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
        std::memcpy(m->val, data_array, rows*cols*sizeof(double));
    } else {
        for (int i=0; i<rows; i++) {
            for (int j=0; j<cols; j++) {
                gretl_matrix_set(m, i, j, *(data_array+i*cols+j)); //data_array[i*cols+j] in c++ makes no difference?
            }
        }
    }
}

static void numpy_array_to_gretl_storage (const pybind11::dict &pydict,
                                          DATASET *dset)
{
    // we copy "data_array" which lives in pydict to dset->Z
    ssize_t rows, cols;
    int fortranarray;
    double *data_array = NULL;

    // the old code: do not delete for a while
    //pybind11::array_t<double> pynum_array = pybind11::cast<pybind11::array>(pydict[pybind11::cast("data_array")]);
    //pybind11::array_t<double> *pynum_array_new = static_cast<pybind11::array_t<double>*> (pybind11::cast<pybind11::array>(pydict[pybind11::cast("data_array")]).mutable_data());
    data_array = numpy_array_extract(pybind11::cast<pybind11::array>(pydict[pybind11::cast("data_array")]),
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
            std::memcpy(dset->Z[i+1], data_array+i*rows, rows*sizeof(double));
        } else {
            for (int j=0; j<rows; j++) {
                *(dset->Z[i+1]+j) = *(data_array+i+j*cols); //data_array[i+j*cols] in c++ makes no difference?
            }
        }
    }
    dset->t1 = 0;
    dset->t2 = dset->n-1;
}

void numpy_dict_to_gretl_dset (const pybind11::dict &pydict,
                               DATASET *dset)
{
    //
    // we fill dset with pydict content
    //
    pybind11::list pylist;
    std::string *strBUF; // maybe we should use vector of strings here?
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
    pylist = pydict[pybind11::cast("obs_labels")];
    if (pylist.size() != (long unsigned int) dset->n) {
        throw std::runtime_error("Number of obs_labels is different than number of observations.");
    }
    copy_strings_from_python_list(pylist, dset->S, OBSLEN-1, 0, 0, true);

    // we fill up dset->varname
    pylist = pydict[pybind11::cast("varname")];
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

    /*
    //some hardwired stuff
    copy_string_from_python_dict(pydict, "stobs", dset->stobs, OBSLEN);
    copy_string_from_python_dict(pydict, "endobs", dset->endobs, OBSLEN);
    dset->sd0 = 1975.100000;
    dset->structure = 1;
    dset->auxiliary = 0; */
}

void gretl_dset_to_numpy_dict (char *fname, pybind11::dict &pydict)
{
    pybind11::list *vnames = new pybind11::list;
    pybind11::list *labels = new pybind11::list;
    DATASET *dset;
    double *tmp;
    int i, j, k;
    int err = 0;

    dset = datainfo_new();
    err = gretl_get_data(fname, dset, OPT_NONE, NULL);
    // FIXME if (err) ...

    // copy across the data values
    pybind11::array_t<double> Z({dset->n, dset->v});
    tmp = (double *) Z.request().ptr;
    for (i=0, k=0; i<dset->n; i++) {
	for (j=0; j<dset->v; j++) {
	    *(tmp+k) = dset->Z[j][i];
	    k++;
	}
    }
    pydict[pybind11::cast("data_array")] = Z;

    // copy across the series names */
    for (i=0; i<dset->v; i++) {
	//vnames->append(strdup(dset->varname[i]));
	vnames->append(dset->varname[i]);
    }
    pydict[pybind11::cast("varname")] = vnames;

    // copy or manufacture obs labels
    if (dset->S != NULL) {
	for (i=0; i<dset->n; i++) {
	    labels->append(dset->S[i]);
	}
    } else {
	char s[64];
	
	for (i=0; i<dset->n; i++) {
	    sprintf(s, "%d", i+1);
	    labels->append(s);
	}
    }
    pydict[pybind11::cast("obs_labels")] = labels;

    // copy dataset periodicity
    pydict[pybind11::cast("periodicity")] = dset->pd;

    destroy_dataset(dset);
}
