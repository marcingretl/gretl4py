#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "gretl_python_data.h"
#include <iostream>

#define F_CONTIGUOUS pybind11::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_
#define is_pynum_fortranarray(m) ((m.flags() & F_CONTIGUOUS))

#define GRETL_PYBIND11_DEBUG 0

static void copy_string_from_python_dict (const pybind11::dict &DICT,
                                          const char *key, char *dest,
                                          int n_bytes, int offset = 0)
{
    std::string stdBUF;

    stdBUF = pybind11::cast<std::string>(DICT[pybind11::cast(key)]); //DICT[pybind11::cast(key)] is of pybind11::str type
    stdBUF.copy(dest, n_bytes, offset);
}

static double *numpy_array_extract (const pybind11::array_t<double> &pynum_array,
                                    ssize_t *rows, ssize_t *cols, int *fortranarray)
{
    // at first we check dimensions
    if (pynum_array.ndim() > 2) {
        throw std::runtime_error("Given array has more than two dimensions.");
    }

    *rows = pynum_array.shape(0);
    *cols = pynum_array.shape(1);
    *fortranarray = is_pynum_fortranarray(pynum_array);

    auto arrBUF = pynum_array.request();
    return (double*) arrBUF.ptr;
}

static void numpy_array_to_gretl_matrix (const double *data_array,
                                         gretl_matrix *m, ssize_t rows,
                                         ssize_t cols, int fortranarray)
{
    //
    // from 'data_array' to 'm' total of rows*cols elements
    //
    if (gretl_matrix_realloc(m, rows, cols)) {
        throw std::runtime_error("Matrix allocation failed.");
    }

    if (fortranarray) {
        std::memcpy(m->val, data_array, rows*cols*sizeof(double));
    } else {
        for (int i=0; i<m->rows; i++) {
            for (int j=0; j<m->cols; j++) {
                gretl_matrix_set(m, i, j, *(data_array+i*rows+j)); //data_array[i*rows+j] in c++ makes no difference?
            }
        }
    }

    #if GRETL_PYBIND11_DEBUG
    std::cout << "rows: " << m->rows << std::endl;
    std::cout << "cols: " << m->cols << std::endl;
    gretl_matrix_print(m, "m (libgretl)");
    #endif // defined
}

void gretl_model_to_dict (const MODEL *model, pybind11::dict &DICT)
{
    pybind11::dict coeff, sderr;

    for (int i=0; i<model->ncoeff; i++) {
        coeff[pybind11::cast(model->params[i])] = model->coeff[i];
        sderr[pybind11::cast(model->params[i])] = model->sderr[i];
        #if GRETL_PYBIND11_DEBUG
        std::cout << "(debug) " << model->params[i] << ": " << model->coeff[i] << " (" << model->sderr[i] << ")" << std::endl;
        #endif // GRETL_PYBIND11_DEBUG
    }

    DICT[pybind11::cast("coeff")] = coeff;
    DICT[pybind11::cast("sderr")] = sderr;
}

void numpy_array_to_gretl_storage (const pybind11::array_t<double> &pynum_array,
                                   gretl_matrix *m)
{
    // we copy pynum_array to empty m
    if (m->rows || m->cols) {
        throw std::runtime_error("Given matrix is not empty.");
    }

    ssize_t rows, cols;
    int fortranarray;
    double *data_array;

    data_array = numpy_array_extract(pynum_array, &rows, &cols, &fortranarray);

    if (data_array == NULL) {
        throw std::runtime_error("Numpy array: null pointer.");
    }
    numpy_array_to_gretl_matrix(data_array, m, rows, cols, fortranarray);
}


void numpy_dict_to_gretl_dset (const pybind11::dict &DICT,
                               DATASET *dset)
{
    // we copy pynum_array to empty m
    char stobs[OBSLEN], endobs[OBSLEN];
    pybind11::array_t<double> pynum_array;
    double *data_array;

   /*
    DICT["varname"] = varname
    DICT["obs_labels"] = obs_labels
    */

    pynum_array = pybind11::cast<pybind11::array>(DICT[pybind11::cast("data_array")]);
    if (pynum_array.ndim() > 2) {
        throw std::runtime_error("Given array has more than two dimensions.");
    }
    dset->n = pynum_array.shape(0);
    dset->v = pynum_array.shape(1) + 1;
    if (allocate_Z(dset, OPT_NONE)) {
        throw std::runtime_error("Matrix allocation failed.");
    }
    auto arrBUF = pynum_array.request();
    data_array = (double*) arrBUF.ptr;
    for (int i=0; i<dset->v-1; i++) {
        std::memcpy(dset->Z[i+1], &data_array[i*dset->n], sizeof(double)*dset->n);
    }

    #if GRETL_PYBIND11_DEBUG
    std::cout << "observations: " << dset->n << std::endl;
    std::cout << "variables: " << dset->v << std::endl;
    #endif // GRETL_PYBIND11_DEBUG

    // we copy strings
    copy_string_from_python_dict(DICT, "stobs", stobs, OBSLEN);
    copy_string_from_python_dict(DICT, "endobs", endobs, OBSLEN);

    /*
    for (pybind11::ssize_t i=0; i<rows; i++) {
        for (pybind11::ssize_t j=0; j<cols; j++) {
            //Z[i+offset][j] = DATA(i,j);
           //gretl_matrix_set(Z, i+offset, j, DATA(i,j));
        }
    }
    */
}

void numpy_array_to_gretl_data (const pybind11::array_t<double> &pynum_array)
{
    int rows = pynum_array.shape(0);
    int cols = pynum_array.shape(1);
    auto DATA = pynum_array.unchecked<2>(); /* only dimensions are checked */
    /*
    if (pynum_array.ndim() != 2) {
        throw std::runtime_error("I expected 2d array (row x cols).");
    }
    auto buf = pynum_array.request();
    double *ptr = (double*) buf.ptr;
    */

    std::cout << "I'm a 'pynum_array' with the following attributes:" << std::endl;
    std::cout << "dimensions: " << pynum_array.ndim() << std::endl;
    std::cout << "number of elements: " << pynum_array.size() << std::endl;
    std::cout << "rows: " << rows << std::endl;
    std::cout << "cols: " << cols << std::endl;
    std::cout << "contents:" << std::endl;
    for (pybind11::ssize_t i=0; i<rows; i++) {
        for (pybind11::ssize_t j=0; j<cols; j++) {
            //printf("[%ld][%ld] = %f ", i, j, ptr[i*cols+j]);
            printf("[%ld][%ld] = %f ", i, j, DATA(i,j));
        }
        std::cout << std::endl;
    }
}
