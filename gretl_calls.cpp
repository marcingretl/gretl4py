#include "gretl_calls.h" /* MUST be at the first position!!! */
#include <gretl/libgretl.h>
#include <iostream>

#define GRETL_PYBIND11_DEBUG 0

static void gretl_model_to_dict (const MODEL *model,
                                 pybind11::dict &DICT)
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

void numpy_array_to_gretl_data (const pybind11::array_t<double> &pynum_array)
{
    auto DATA = pynum_array.unchecked<2>();
    /*
    if (pynum_array.ndim() != 2) {
        throw std::runtime_error("I expected 2d array (row x cols).");
    }
    auto buf = pynum_array.request();
    double *ptr = (double*) buf.ptr;
    */

    int rows = pynum_array.shape(0);
    int cols = pynum_array.shape(1);

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

pybind11::dict arma_estimate (int p, int d, int q,
                              bool do_print = false)
{
    pybind11::dict res;
    DATASET *dset;
    MODEL *model;
    PRN *prn = NULL;
    int *parmlist;
    int err;

    dset = datainfo_new();
    model = gretl_model_new();
    parmlist = gretl_list_new(5);
    if (do_print) {
        prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);
    }

    err = gretl_read_native_data("data9-7.gdt", dset);

    parmlist[1] = p;        /* AR order */
    parmlist[2] = d;        /* order of integration */
    parmlist[3] = q;        /* MA order */
    parmlist[4] = LISTSEP;  /* separator */
    parmlist[5] = 1;        /* position of dependent variable in dataset */

    *model = arma(parmlist, NULL, dset, OPT_NONE, prn);
    err = model->errcode;

    if (err) {
        errmsg(err, prn);
    } else {
        gretl_model_to_dict(model, res);
        if (do_print) {
            printmodel(model, dset, OPT_NONE, prn);
        }
    }

    gretl_model_free(model);
    free(parmlist);
    destroy_dataset(dset);
    gretl_print_destroy(prn);

    return res;
}
