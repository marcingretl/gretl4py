#include "gretl_calls.h" /* MUST be at the first position!!! */
#include "gretl_python_data.h"
#include <iostream>

#define GRETL_PYBIND11_DEBUG 0

static DATASET *DSET = NULL; // global dataset

void module_init ()
{
    libgretl_init();
    std::cout << "libgretl initialized\n" << std::endl;
}

void module_cleanup ()
{
    libgretl_cleanup();
    std::cout << "\nlibgretl cleaned up" << std::endl;
}

void about (const std::string &info) {
    lib_logo();
    std::cout << "module ver: " << GRETL_MODULE_VERSION << std::endl;
    std::cout << info << std::endl;
}

// two overloaded
void load_data (const pybind11::array_t<double> &pynum_array)
{
    //
    // interface for loading pure Numpy arrays (overloaded)
    //
    gretl_matrix *m = gretl_null_matrix_new();
    numpy_array_to_gretl_storage(pynum_array, m);
    gretl_matrix_print(m, "m (libgretl)");
    gretl_matrix_free(m);
}

void load_data (const pybind11::dict &pydict)
{
    //
    // interface for loading rich data from dictionary (overloaded)
    //
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

static void do_arma (DATASET *dset, const std::string y,
                     int p, int d, int q,
                     bool print, pybind11::dict &res)
{
    //
    // arma estimation with two overloaded interfaces beneath
    //
    MODEL *model;
    PRN *prn = NULL;
    int *parmlist;
    int err;
    int y_number = current_series_index(dset, y.c_str());

    if (y_number == -1) {
        delete &res;
        throw std::runtime_error(std::string("There is no variable: ") + y);
    }
    model = gretl_model_new();
    parmlist = gretl_list_new(5);
    if (print) {
        prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);
    }

    parmlist[1] = p;        /* AR order */
    parmlist[2] = d;        /* order of integration */
    parmlist[3] = q;        /* MA order */
    parmlist[4] = LISTSEP;  /* separator */
    parmlist[5] = y_number; /* position of dependent variable in dataset */

    *model = arma(parmlist, NULL, dset, OPT_NONE, prn);
    err = model->errcode;

    if (err) {
        errmsg(err, prn);
    } else {
        gretl_model_to_dict(model, dset, res);
        if (print) {
            printmodel(model, dset, OPT_NONE, prn);
        }
    }

    gretl_model_free(model);
    free(parmlist);
    gretl_print_destroy(prn);
}

pybind11::dict *arma (const pybind11::dict &data,
                      const std::string y,
                      int p, int d, int q, bool print)
{
    //
    // interface for arma estimation on bundled data (overloaded)
    //
    pybind11::dict *res = new pybind11::dict;
    DATASET *dset = datainfo_new();

    numpy_dict_to_gretl_dset(data, dset);
    do_arma(dset, y, p, d, q, print, *res);

    destroy_dataset(dset);

    return res;
}

pybind11::dict *arma (const std::string y,
                      int p, int d, int q, bool print)
{
    //
    // interface for arma estimation on global dataset (overloaded)
    //
    pybind11::dict *res = new pybind11::dict;

    if (DSET == NULL) {
        throw std::runtime_error("No dataset in place");
    }
    do_arma(DSET, y, p, d, q, print, *res);

    return res;
}

static void do_ols (DATASET *dset, const std::string y,
		    const std::string x1, const std::string x2,
		    bool print, pybind11::dict &res)
{
    MODEL *model;
    PRN *prn = NULL;
    int *parmlist;
    int err;

    model = gretl_model_new();
    parmlist = gretl_list_new(4);
    if (print) {
        prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);
    }

    parmlist[1] = current_series_index(dset, y.c_str());
    parmlist[2] = 0;
    parmlist[3] = current_series_index(dset, x1.c_str());
    parmlist[4] = current_series_index(dset, x2.c_str());

    *model = lsq(parmlist, dset, OLS, OPT_NONE);
    err = model->errcode;

    if (err) {
        errmsg(err, prn);
    } else {
        gretl_model_to_dict(model, dset, res);
        if (print) {
            printmodel(model, dset, OPT_NONE, prn);
        }
    }

    gretl_model_free(model);
    free(parmlist);
    gretl_print_destroy(prn);
}

pybind11::dict *ols (const pybind11::dict &data,
		     const std::string y,
		     const std::string x1,
		     const std::string x2,
		     bool print)
{
    pybind11::dict *res = new pybind11::dict;
    DATASET *dset = datainfo_new();

    numpy_dict_to_gretl_dset(data, dset);
    do_ols(dset, y, x1, x2, print, *res);

    return res;
}
