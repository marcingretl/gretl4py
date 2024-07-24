#include "gretl_calls.h" /* MUST be at the first position!!! */
#include "gretl_python_data.h"
#include <gretl/gretl_drivers.h>
#include <gretl/gretl_errors.h>
#include <gretl/libset.h>
#include <iostream>

#define GRETL_PYBIND11_DEBUG 0

static DATASET *DSET = NULL; // global dataset

// module related tools

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

static void handle_gretl_error (int err)
{
    const char *msg = errmsg_get_with_default(err);
    throw std::runtime_error(msg);
}

// set a string-valued "setvar"

void set (const std::string key, const std::string val)
{
    int err = execute_set(key.c_str(), val.c_str(), NULL, OPT_NONE, NULL);

    if (err) {
        handle_gretl_error(err);
    }
}

// set an integer-valued "setvar"

void set (const std::string key, int val)
{
    char sval[32];
    int err;

    sprintf(sval, "%d", val);
    err = execute_set(key.c_str(), sval, NULL, OPT_NONE, NULL);
    if (err) {
        handle_gretl_error(err);
    }
}

// report a current "setvar" setting

void set (const std::string key)
{
    PRN *prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);
    int err = execute_set(key.c_str(), NULL, NULL, OPT_NONE, prn);

    if (err) {
        handle_gretl_error(err);
    }
}

// data management

// interface for loading pure Numpy arrays
void load_data (const py::array_t<double> &pynum_array)
{
    gretl_matrix *m = gretl_null_matrix_new();

    numpy_array_to_gretl_storage(pynum_array, m);
    gretl_matrix_print(m, "m (libgretl)");
    gretl_matrix_free(m);
}

// interface for loading data from dictionary

void load_data (const py::dict &pydict)
{
    if (DSET != NULL) {
        destroy_dataset(DSET);
    }
    DSET = datainfo_new();
    numpy_dict_to_gretl_dset(pydict, DSET);
}

static void do_printdata (DATASET *dset, const std::string *varnames,
                          const std::string *ostr, gretlopt opt,
                          bool varnames_only = false)
{
    int *varlist = NULL;
    int err;
    PRN *prn = 0;

    if (varnames == NULL) {
        varlist = gretl_list_new(dset->v);
        for (int i=1; i<dset->v; i++) {
            if (varnames_only) {
                std::printf("%d %s\n", i, dset->varname[i]);
            } else {
                varlist[i] = current_series_index(dset, dset->varname[i]);
            }
        }
    }

    if (!varnames_only) {
        prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);
        err = printdata(varlist, NULL, dset, opt, prn);
        gretl_print_destroy(prn);
        if (err) {
            throw std::runtime_error("libgretl printdata() failed.");
        }
    }
}

void printdata (const py::dict &data, bool varamesonly)
{
    DATASET *dset = datainfo_new();

    numpy_dict_to_gretl_dset(data, dset);

    do_printdata(dset, NULL, NULL, OPT_O, varamesonly);
    destroy_dataset(dset);
}

void printdata (bool varnames_only)
{
    if (DSET == NULL) {
        throw std::runtime_error("No dataset in place");
    }

    do_printdata(DSET, NULL, NULL, OPT_O, varnames_only);
}

py::dict *get_gretl_data (const std::string fname)
{
    py::dict *res = new py::dict;
    char tmp[2048];

    strcpy(tmp, fname.c_str());
    gretl_dset_to_numpy_dict(tmp, *res);
    return res;
}

// model estimation

// arma estimation with two overloaded interfaces beneath

static void do_arma (DATASET *dset, const std::string y,
                     int p, int d, int q,
                     bool print, py::dict &res)
{
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

// interface for arma estimation on specified data

py::dict *arma (const py::dict &data,
                const std::string y,
                int p, int d, int q, bool print)
{
    py::dict *res = new py::dict;
    DATASET *dset = datainfo_new();

    numpy_dict_to_gretl_dset(data, dset);
    do_arma(dset, y, p, d, q, print, *res);

    destroy_dataset(dset);

    return res;
}

// interface for arma estimation on global dataset

py::dict *arma (const std::string y,
                int p, int d, int q, bool print)
{
    py::dict *res = new py::dict;

    if (DSET == NULL) {
        throw std::runtime_error("No dataset in place");
    }
    do_arma(DSET, y, p, d, q, print, *res);

    return res;
}

// convert from a simple R-style formula to a gretl list

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

// "Proof of concept" function for getting the options for
//  a specified gretl command

static gretlopt options_from_python_dict (py::dict &d, int ci)
{
    const char *cmd = gretl_command_word(ci);
    int opt, opts = OPT_NONE;
    OptStatus status;

    for (auto &item : d) {
        if (!py::isinstance<py::str>(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();
        printf(" key: %s\n", key.c_str());

        // check validity of @opt for @ci
        opt = valid_long_opt(ci, key.c_str(), &status);
        if (opt) {
            printf("  option is OK for %s, status %d\n", cmd, status);
            opts |= opt;
        } else {
            printf("  option is NOT OK for %s\n", cmd);
        }
        if (py::isinstance<py::str>(item.second)) {
            const std::string &sval = item.second.cast<std::string>();
            printf("  value is string: %s\n", sval.c_str());
        } else if (py::isinstance<py::int_>(item.second)) {
            int ival = item.second.cast<int>();
            printf("  value is int: %d\n", ival);
        } else if (py::isinstance<py::float_>(item.second)) {
            double dval = item.second.cast<double>();
            printf("  value is float: %g\n", dval);
        } else {
            throw std::runtime_error("unsupported option param type");
        }
    }

    return (gretlopt) opts;
}

// temporary driver for options_from_python_dict()

void dict2opts (py::dict &d)
{
    options_from_python_dict(d, OLS);
}

// "Proof of concept" function for supporting more than one
//  estimation method

static void regression_wrapper (int ci, const std::string formula,
                                bool print, DATASET *dset,
                                py::dict &res)
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

    if (ci == OLS) {
        *model = lsq(list, dset, OLS, OPT_NONE);
    } else if (ci == LOGIT || ci == PROBIT) {
        *model = logit_probit(list, dset, ci, OPT_NONE, prn);
    } else {
        throw std::runtime_error("Unsupported estimator");
    }

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
    gretl_model_free(model);
}

// intermediate wrappers to handle specified versus global data

static py::dict
*regress_with_specified_data (int ci, const std::string formula,
                              const py::dict &data, bool print)
{
    py::dict *res = new py::dict;
    DATASET *dset = datainfo_new();

    numpy_dict_to_gretl_dset(data, dset);
    regression_wrapper(ci, formula, print, dset, *res);
    destroy_dataset(dset);

    return res;
}

py::dict *regress_with_global_data (int ci,
                                    const std::string formula,
                                    bool print)
{
    py::dict *res = new py::dict;

    if (DSET == NULL) {
        throw std::runtime_error("No dataset in place");
    } else {
        regression_wrapper(ci, formula, print, DSET, *res);
    }

    return res;
}

// thin wrappers for specific estimators

py::dict *ols (const std::string formula,
               const py::dict &data, bool print)
{
    return regress_with_specified_data(OLS, formula, data, print);
}

py::dict *ols (const std::string formula, bool print)
{
    return regress_with_global_data(OLS, formula, print);
}

py::dict *logit (const std::string formula,
                 const py::dict &data, bool print)
{
    return regress_with_specified_data(LOGIT, formula, data, print);
}

py::dict *probit (const std::string formula,
                  const py::dict &data, bool print)
{
    return regress_with_specified_data(PROBIT, formula, data, print);
}
