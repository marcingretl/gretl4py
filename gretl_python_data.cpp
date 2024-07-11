#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "gretl_python_data.h"
#include <iostream>

#define F_CONTIGUOUS pybind11::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_
#define is_pynum_fortranarray(m) ((m.flags() & F_CONTIGUOUS))

#define GRETL_PYBIND11_DEBUG 1

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

static void copy_string_from_python_dict (const pybind11::dict &DICT,
                                          const char *key, char *dest,
                                          int n_bytes, int offset = 0)
{
    std::string stdBUF;

    stdBUF = pybind11::cast<std::string>(DICT[pybind11::cast(key)]); //DICT[pybind11::cast(key)] is of pybind11::str type
    stdBUF.copy(dest, n_bytes, offset);
}

static void copy_strings_from_python_list (const pybind11::list &LIST,
                                           char **dest, int n_bytes,
                                           int src_offset = 0,
                                           int dst_offset = 0)
{
    std::string stdBUF;
    int i = dst_offset;

    for (auto item : LIST) {
        stdBUF = std::string(pybind11::str(item));
        stdBUF.copy(dest[i++], n_bytes, src_offset);
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

static void numpy_array_to_gretl_storage (const pybind11::dict &DICT, DATASET *dset)
{
    ssize_t rows, cols;
    int fortranarray;
    double *data_array = NULL;
    pybind11::array_t<double> pynum_array;

    pynum_array = pybind11::cast<pybind11::array>(DICT[pybind11::cast("data_array")]);

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
            std::memcpy(dset->Z[i+1], &data_array[i*rows], rows*sizeof(double));
        } else {
            for (int j=0; j<rows; j++) {
                dset->Z[i+1][j] = *(data_array+i+j*cols); //data_array[i+j*cols] in c++ makes no difference?
            }
        }
    }

    #if GRETL_PYBIND11_DEBUG
    std::cout << "observations: " << dset->n << std::endl;
    std::cout << "variables: " << dset->v << std::endl;
    std::cout << "Z[v=1][obs=0] = " << dset->Z[1][0] << std::endl;
    std::cout << "Z[v=1][obs=n-1] = " << dset->Z[1][dset->n-1] << std::endl;
    #endif // GRETL_PYBIND11_DEBUG
}

void numpy_dict_to_gretl_dset (const pybind11::dict &DICT,
                               DATASET *dset)
{
    pybind11::list LIST;

    numpy_array_to_gretl_storage(DICT, dset);

    if (dataset_allocate_varnames(dset) || dataset_allocate_obs_markers(dset)) {
        throw std::runtime_error("Dataset strings allocation failed.");
    }

    LIST = DICT[pybind11::cast("varname")];
    if (LIST.size() != (long unsigned int) dset->v-1) {
        throw std::runtime_error("Number of varnames is different than number of variables.");
    }
    copy_strings_from_python_list(LIST, dset->varname, 32, 0, 1);

    LIST = DICT[pybind11::cast("obs_labels")];
    if (LIST.size() != (long unsigned int) dset->n) {
        throw std::runtime_error("Number of obs_labels is different than number of observations.");
    }
    copy_strings_from_python_list(LIST, dset->S, OBSLEN-1, 0, 0);


    //some hardwired stuff
    copy_string_from_python_dict(DICT, "stobs", dset->stobs, OBSLEN);
    copy_string_from_python_dict(DICT, "endobs", dset->endobs, OBSLEN);
    dset->sd0 = 1975.100000;
    dset->structure = 1;
    dset->t1 = 0;
    dset->t2 = 63;
}
