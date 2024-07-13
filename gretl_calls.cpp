#include "gretl_calls.h" /* MUST be at the first position!!! */
#include "gretl_python_data.h"
#include <iostream>

#define GRETL_PYBIND11_DEBUG 0

static DATASET *DSET = NULL; // global dataset

void about (const char *info) {
    lib_logo();
    std::cout << info << std::endl;
}

void load_data (const pybind11::dict &pydict)
{
    if (DSET != NULL) {
        destroy_dataset(DSET);
    }
    DSET = datainfo_new();
    numpy_dict_to_gretl_dset(pydict, DSET);

    #if 0 //GRETL_PYBIND11_DEBUG
    std::cout << "rows: " << m->rows << std::endl;
    std::cout << "cols: " << m->cols << std::endl;
    gretl_matrix_print(m, "m (libgretl)");
    #endif // defined
}

static void do_arma (DATASET *dset, int p, int d, int q,
                     bool print, pybind11::dict &res)
{
    MODEL *model;
    PRN *prn = NULL;
    int *parmlist;
    int err;

    model = gretl_model_new();
    parmlist = gretl_list_new(5);
    if (print) {
        prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);
    }

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
        if (print) {
            printmodel(model, dset, OPT_NONE, prn);
        }
    }

    gretl_model_free(model);
    free(parmlist);
    gretl_print_destroy(prn);
}
pybind11::dict arma (const pybind11::dict &data,
                     int p, int d, int q, bool print)
{
    pybind11::dict res;
    DATASET *dset = datainfo_new();

    numpy_dict_to_gretl_dset(data, dset);

    do_arma(dset, p, d, q, print, res);

    destroy_dataset(dset);

    return res;
}
pybind11::dict arma (int p, int d, int q, bool print)
{
    pybind11::dict res;

    if (DSET == NULL) {
        throw std::runtime_error("No dataset in place");
    }

    do_arma(DSET, p, d, q, print, res);

    return res;
}

/*
pybind11::dict arma_estimate_old (int p, int d, int q,
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
*/

