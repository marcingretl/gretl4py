#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
//#include <pybind11/stl.h>
#include "gretl_python_data.h"
#include <iostream>
#include <string>

#define F_CONTIGUOUS pybind11::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_
#define is_pynum_fortranarray(m) ((m.flags() & F_CONTIGUOUS))

#define GRETL_PYBIND11_DEBUG 0

void gretl_model_to_dict (const MODEL *model, pybind11::dict &pydict)
{
    //
    // here we construct python's dict which is returned with estimation results
    //
    pybind11::dict coeff, sderr;

    for (int i=0; i<model->ncoeff; i++) {
        coeff[pybind11::cast(model->params[i])] = model->coeff[i];
        sderr[pybind11::cast(model->params[i])] = model->sderr[i];
        #if GRETL_PYBIND11_DEBUG
        std::cout << "(debug) " << model->params[i] << ": " << model->coeff[i] << " (" << model->sderr[i] << ")" << std::endl;
        #endif // GRETL_PYBIND11_DEBUG
    }

    pydict[pybind11::cast("coeff")] = coeff;
    pydict[pybind11::cast("sderr")] = sderr;
    pydict[pybind11::cast("lnL")] = model->lnL;
}

static void copy_string_from_python_dict (const pybind11::dict &pydict,
                                          const char *key, char *dest,
                                          int n_bytes, int offset = 0)
{
    //
    // here we take just one element (under the "key") and copy do dest
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

static void copy_strings_from_python_list (const pybind11::list &pylist,
                                           char **dest, int n_bytes,
                                           int src_offset = 0,
                                           int dst_offset = 0)
{
    //
    // we copy the whole list to dest (length must be verified at the caller level)
    //
    std::string stdBUF;
    int i = dst_offset;

    for (auto item : pylist) {
        stdBUF = std::string(pybind11::str(item));
        stdBUF.erase(std::remove_if(stdBUF.begin(), stdBUF.end(), ::isspace), stdBUF.end());
        stdBUF.copy(dest[i++], n_bytes, src_offset);
    }
}
static void copy_strings_from_python_list2 (const pybind11::list &pylist,
                                            std::string *stdBUF,
                                            bool trim = false)
{
    //
    // we copy the whole list to stdBUF (length must be verified at the caller level)
    //
    int i = 0;

    for (auto item : pylist) {
        stdBUF[i++] = std::string(pybind11::str(item));
    }
    if (trim) {
        //stdBUF.erase(std::remove_if(stdBUF.begin(), stdBUF.end(), ::isspace), stdBUF.end());
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

    // FIXME: we should try to use *ptr here
    auto arrBUF = pynum_array.request();
    return (double*) arrBUF.ptr;
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

    #if GRETL_PYBIND11_DEBUG
    std::cout << "rows: " << m->rows << std::endl;
    std::cout << "cols: " << m->cols << std::endl;
    gretl_matrix_print(m, "m (libgretl)");
    #endif // defined
}

static void numpy_array_to_gretl_storage (const pybind11::dict &pydict,
                                          DATASET *dset)
{
    // we copy "data_array" which lives in pydict to dset->Z
    ssize_t rows, cols;
    int fortranarray;
    double *data_array = NULL;
    pybind11::array_t<double> pynum_array;

    // FIXME me: we want *ptr here, maybe unique_ptr/shared_ptr?
    pynum_array = pybind11::cast<pybind11::array>(pydict[pybind11::cast("data_array")]);

    data_array = numpy_array_extract(pynum_array, &rows, &cols, &fortranarray);
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
    dset->t2 = dset->n - 1;

    #if GRETL_PYBIND11_DEBUG
    std::cout << "observations: " << dset->n << std::endl;
    std::cout << "variables (including const): " << dset->v << std::endl;
    std::cout << "Z[v=1][obs=0] = " << dset->Z[1][0] << std::endl;
    std::cout << "Z[v=1][obs=n-1] = " << dset->Z[1][dset->n-1] << std::endl;
    #endif // GRETL_PYBIND11_DEBUG
}

static void copy_strings_to_char_array (const std::string *strBUF,
                                           char **dest, int n,
                                           const char *for_dset = NULL,
                                           int src_offset = 0,
                                           int dst_offset = 0)
{
    //
    // we copy the whole stdBUF to dest (length must be verified at the caller level)
    // 'for_dset' indicates two spacial cases, namelly: dset->varname and dset->S
    //

    char *BUF = NULL;
    int BUFsize = 0;

    if (!std::strcmp(for_dset, "dset->varname")) {
        BUFsize = VNAMELEN;
    } else if (!std::strcmp(for_dset, "dset->S")) {
        BUFsize = OBSLEN-1;
    } else {
        BUFsize = MAXLEN;
    }

    BUF = new char[BUFsize];

    for (int i=0; i<n-1; i++) {
        BUF[0] = '\0';
        strBUF[i].copy(BUF, BUFsize, 0); // maybe std::strcpy would be better
        if (!std::strcmp(for_dset, "dset->varname")) {
            gretl_normalize_varname(BUF, strBUF[i].c_str(), 0, 0);
        }
        memset(dest[i+dst_offset], '\0', sizeof(dest));
        strBUF[i].copy(dest[i+dst_offset], BUFsize-1, src_offset);
        std::cout << i << " " << strBUF[i];
        std::cout << BUF << std::endl;
    }

    delete []BUF;
}

void numpy_dict_to_gretl_dset (const pybind11::dict &pydict,
                               DATASET *dset)
{
    //
    // we fill dset with pydict content
    //
    pybind11::list pylist;
    std::string *strBUF; // maybe we would use vector of strings here
    char periodicity[16];

    if (dset == NULL) {
        throw std::runtime_error("Dataset strings allocation failed.");
    }

    numpy_array_to_gretl_storage(pydict, dset);

    if (dataset_allocate_varnames(dset) || dataset_allocate_obs_markers(dset)) {
        throw std::runtime_error("Dataset strings allocation failed.");
    }

    /*
    pylist = pydict[pybind11::cast("varname")];
    if (pylist.size() != (long unsigned int) dset->v-1) {
        throw std::runtime_error("Number of varnames is different than number of variables.");
    }
    copy_strings_from_python_list(pylist, dset->varname, VNAMELEN, 0, 1);
    */

    pylist = pydict[pybind11::cast("obs_labels")];
    if (pylist.size() != (long unsigned int) dset->n) {
        throw std::runtime_error("Number of obs_labels is different than number of observations.");
    }
    copy_strings_from_python_list(pylist, dset->S, OBSLEN-1, 0, 0);


    strBUF = new std::string[dset->v];
    pylist = pydict[pybind11::cast("varname")];
    if (pylist.size() != (long unsigned int) dset->v-1) {
        throw std::runtime_error("Number of varnames is different than number of variables.");
    }
    copy_strings_from_python_list2(pylist, strBUF);
    copy_strings_to_char_array(strBUF, dset->varname, dset->v, "dset->varname", 0, 1);
    delete []strBUF;
    for (int i=0; i<dset->v; i++)
        printf("(debug) %i: %s\n", i, dset->varname[i]);

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
