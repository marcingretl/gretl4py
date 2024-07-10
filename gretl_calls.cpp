#include "gretl_calls.h" /* MUST be at the first position!!! */
#include "gretl_python_data.h"
#include <iostream>

#define GRETL_PYBIND11_DEBUG 1

void dsettest (const pybind11::dict &DICT)
{
    DATASET *dset, *dsetnew;
    int *parmlist;

    dset = datainfo_new();
    dsetnew = datainfo_new();
    parmlist = gretl_list_new(3);
    PRN *prn;
    gretl_read_native_data("data9-7.gdt", dset);
    prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);

    parmlist[1] = 1;
    parmlist[2] = 2;
    parmlist[3] = 3;


    printf("the first observation of vaiable 1: %f\n", dset->Z[1][0]);
    printf("the first observation of vaiable 3: %f\n", (double) dset->Z[3][0]);
    printf("pierwsza etykieta obserwacji: %s\n", dset->stobs);
    printf("ostatnia etykieta obserwacji: %s\n", dset->endobs);
    printf("structure: %d\n", dset->structure);
    printf("sd0: %f\n", dset->sd0);
    printf("t1: %d\n", dset->t1);
    printf("t2: %d\n", dset->t2);


    print_data_in_columns(parmlist, NULL, dset, OPT_NONE, prn);

    numpy_dict_to_gretl_dset(DICT, dsetnew);
    print_data_in_columns(parmlist, NULL, dsetnew, OPT_NONE, prn);


    free(parmlist);
    destroy_dataset(dset);
    gretl_print_destroy(prn);
}

void load_matrix (const pybind11::array_t<double> &pynum_array)
{
    gretl_matrix *m = gretl_null_matrix_new();
    numpy_array_to_gretl_storage(pynum_array, m);
    gretl_matrix_free(m);
}

void load_data (const pybind11::dict &DICT)
{
    //gretl_matrix *m = gretl_null_matrix_new();
    //numpy_array_to_gretl_matrix(pynum_array, m);

    numpy_dict_to_gretl_dset(DICT, NULL);

    #if 0 //GRETL_PYBIND11_DEBUG
    std::cout << "rows: " << m->rows << std::endl;
    std::cout << "cols: " << m->cols << std::endl;
    gretl_matrix_print(m, "m (libgretl)");
    #endif // defined
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
