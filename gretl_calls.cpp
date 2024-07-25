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

void printdata (const py::dict &data, bool namesonly)
{
    DATASET *dset = datainfo_new();

    numpy_dict_to_gretl_dset(data, dset);

    do_printdata(dset, NULL, NULL, OPT_O, namesonly);
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

static gretlopt unpack_kwargs (const py::kwargs &kwargs,
                               int ci, int *do_print,
                               DATASET **pdset)
{
    int opt, opts = OPT_NONE;
    OptStatus status;
    bool got_data = false;
    char numstr[32];

    for (auto &item : kwargs) {
        bool is_print = false;

        if (!py::isinstance<py::str>(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (!strcmp(key.c_str(), "data")) {
            // special
            got_data = true;
            if (py::isinstance<py::dict>(item.second)) {
                const py::dict data = item.second.cast<py::dict>();

                *pdset = datainfo_new();
                numpy_dict_to_gretl_dset(data, *pdset);
            } else {
                throw std::runtime_error("'data' must be a dictionary");
            }
            continue;
        } if (!strcmp(key.c_str(), "print")) {
            // special
            is_print = true;
            status = OPT_NO_PARM;
        } else {
            // a gretl option: check its validity in the context of @ci
            opt = valid_long_opt(ci, key.c_str(), &status);
            if (opt == 0) {
                throw std::runtime_error(std::string("invalid option flag ") + key);
            }
        }
        if (status == OPT_NO_PARM) {
            // no parameter wanted, but we'll accept a boolean
            if (py::isinstance<py::int_>(item.second)) {
                int ival = item.second.cast<int>();

                if (ival == 1) {
                    if (is_print) {
                        *do_print = 1;
                    } else {
                        opts |= opt;
                    }
                } else if (ival != 0) {
                    throw std::runtime_error("invalid option parameter");
                }
            }
        } else {
            // parameter accepted, or required
            if (py::isinstance<py::str>(item.second)) {
                const std::string &sval = item.second.cast<std::string>();

                push_option_param(ci, (gretlopt) opt, gretl_strdup(sval.c_str()));
             } else if (py::isinstance<py::int_>(item.second)) {
                int ival = item.second.cast<int>();

                sprintf(numstr, "%d", ival);
                push_option_param(ci, (gretlopt) opt, gretl_strdup(numstr));
            } else if (py::isinstance<py::float_>(item.second)) {
                double dval = item.second.cast<double>();

                sprintf(numstr, "%g", (double) dval);
                push_option_param(ci, (gretlopt) opt, gretl_strdup(numstr));
            } else {
                throw std::runtime_error("unsupported option param type");
            }
        }
    }

    if (!got_data) {
        *pdset = DSET;
    }

    return (gretlopt) opts;
}

// "Proof of concept" function for supporting more than one
//  estimation method

static py::dict *regression_wrapper (const std::string formula,
                                     const py::kwargs &kwargs,
                                     int ci)
{
    py::dict *res = new py::dict;
    DATASET *dset = NULL;
    MODEL *model = NULL;
    PRN *prn = NULL;
    int *list = NULL;
    gretlopt opt = OPT_NONE;
    int do_print = 0;
    int err = 0;

    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs(kwargs, ci, &do_print, &dset);
    } else {
        dset = DSET;
    }

    if (dset == NULL) {
        throw std::runtime_error("No dataset is available");
    }

    list = list_from_formula(formula.c_str(), dset);
    model = gretl_model_new();
    if (do_print) {
        prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);
    }

    if (ci == OLS) {
        *model = lsq(list, dset, OLS, opt);
    } else if (ci == LOGIT || ci == PROBIT) {
        *model = logit_probit(list, dset, ci, opt, prn);
    } else {
        throw std::runtime_error("Unsupported estimator");
    }

    err = model->errcode;
    if (err) {
        errmsg(err, prn);
    } else {
        gretl_model_to_dict(model, dset, *res);
        if (do_print) {
            printmodel(model, dset, OPT_NONE, prn);
        }
    }

    if (dset != DSET) {
        destroy_dataset(dset);
    }

    free(list);
    gretl_print_destroy(prn);
    gretl_model_free(model);

    return res;
}

// thin wrappers for specific estimators

py::dict *ols (const std::string formula,
               const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, OLS);
}

py::dict *logit (const std::string formula,
                 const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, LOGIT);
}

py::dict *probit (const std::string formula,
                  const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, PROBIT);
}

static void pass_kwargs (const py::kwargs &kwargs)
{
    fprintf(stderr, "in pass_kwargs\n");
    for (auto &item : kwargs) {
        const std::string &key = item.first.cast<std::string>();

        fprintf(stderr, " key: %s\n", key.c_str());
    }
}

void kwtest (const std::string formula,
             const py::kwargs &kwargs)
{
    int printval = 0;

    for (auto &item : kwargs) {
        const std::string &key = item.first.cast<std::string>();

        if (key == "print") {
            printval = item.second.cast<int>();
        } else if (key == "data") {
            fprintf(stderr, "got data argument\n");
        }
    }
    fprintf(stderr, "printval = %d\n", printval);
    pass_kwargs(kwargs);
}
