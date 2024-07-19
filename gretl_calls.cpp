#include "gretl_calls.h" /* MUST be at the first position!!! */
#include "gretl_python_data.h"
#include <iostream>

#define GRETL_PYBIND11_DEBUG 0

static DATASET *DSET = NULL; // global dataset

void module_init ()
{
    libgretl_init();
    std::cout << "libgretl initialized";
    std::cout  << std::endl << std::endl;
}

void module_cleanup ()
{
    libgretl_cleanup();
    std::cout << std::endl << "libgretl cleaned up" << std::endl;
}

void about () {
    lib_logo();
    std::cout << "module ver: " << GRETL_MODULE_VERSION << std::endl;
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
}

pybind11::dict *get_gretl_data (const std::string fname)
{
    pybind11::dict *res = new pybind11::dict;
    char tmp[2048];

    strcpy(tmp, fname.c_str());
    gretl_dset_to_numpy_dict(tmp, *res);
    return res;
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

static int *list_from_formula (const char *formula,
			       DATASET *dset)
{
    char *tmp = gretl_strdup(formula);
    int *list;
    int err = 0;

    gretl_charsub(tmp, '~', ' ');
    gretl_charsub(tmp, '+', ' ');
    list = generate_list(tmp, dset, 0, &err);
    free(tmp);

    return list;
}

static void ols_wrapper (const std::string formula,
			 bool print,
			 DATASET *dset,
			 pybind11::dict &res)
{
    MODEL *model = NULL;
    PRN *prn = NULL;
    int *list = NULL;
    int err = 0;

    list = list_from_formula(formula.c_str(), dset);
    model = gretl_model_new();
    if (print) {
	prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);
    }
    
    *model = lsq(list, dset, OLS, OPT_NONE);
    err = model->errcode;
    if (err) {
	errmsg(err, prn);
    } else {
	gretl_model_to_dict(model, dset, res);
	if (print) {
	    printmodel(model, dset, OPT_NONE, prn);
	}
    }	

    free(list);
    gretl_print_destroy(prn);
}

pybind11::dict *ols (const std::string formula,
		     const pybind11::dict &data,
		     bool print)
{
    pybind11::dict *res = new pybind11::dict;
    DATASET *dset = datainfo_new();
    
    numpy_dict_to_gretl_dset(data, dset);
    ols_wrapper(formula, print, dset, *res);
    destroy_dataset(dset);
    
    return res;
}

pybind11::dict *ols (const std::string formula, bool print)
{
    pybind11::dict *res = new pybind11::dict;
    
    if (DSET == NULL) {
	throw std::runtime_error("No dataset in place");
    } else {
	ols_wrapper(formula, print, DSET, *res);
    }

    return res;
}
