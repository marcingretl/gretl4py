#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "gretl_python_data.h"
#include <iostream>

#define GRETL_PYBIND11_DEBUG 0

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

void numpy_array_to_gretl_matrix (const pybind11::array_t<double> &pynum_array,
                                  gretl_matrix *m)
{
    // we copy pynum_array to empty m

    if (m->rows || m->cols) {
        throw std::runtime_error("Given matrix is not empty.");
    }

    auto DATA = pynum_array.unchecked<2>();
    if (gretl_matrix_realloc(m, pynum_array.shape(0),
                              pynum_array.shape(1))) {
        throw std::runtime_error("Matrix allocation failed.");
    }

    for (pybind11::ssize_t i=0; i<m->rows; i++) {
        for (pybind11::ssize_t j=0; j<m->cols; j++) {
            gretl_matrix_set(m, i, j, DATA(i,j));
        }
    }
    #if GRETL_PYBIND11_DEBUG
    std::cout << "rows: " << m->rows << std::endl;
    std::cout << "cols: " << m->cols << std::endl;
    gretl_matrix_print(m, "m (libgretl)");
    #endif // defined
}

void numpy_dict_to_gretl_dstet (const pybind11::dict DICT)
{
    // we copy pynum_array to empty m
    DATASET *dset;
    dset = datainfo_new();

    char stobs[OBSLEN], endobs[OBSLEN];
    pybind11::str pyBUF;
    std::string stdBUF;

   /*
    DICT["data_array"] = data_array
    DICT["varname"] = varname
    DICT["obs_labels"] = obs_labels
    DICT["stobs"] = stobs
    DICT["endobs"] = endobs
    */

    pyBUF = DICT[pybind11::cast("stobs")];
    stdBUF = pybind11::cast<std::string>(pyBUF);
    stdBUF.copy(stobs, OBSLEN, 0);

    pyBUF = DICT[pybind11::cast("endobs")];
    stdBUF = pybind11::cast<std::string>(pyBUF);
    stdBUF.copy(endobs, OBSLEN, 0);

    printf("stobs: %s\n", stobs);
    printf("endobs: %s\n", endobs);
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
