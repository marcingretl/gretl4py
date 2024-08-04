#include "gretl_calls.h" /* MUST be at the first position!!! */
#include "gretl_python_data.h"
#include <gretl/gretl_drivers.h>
#include <gretl/gretl_errors.h>
#include <gretl/libset.h>
#include <iostream>

#define GRETL_PYBIND11_DEBUG 0

static DATASET *DSET = NULL; // global dataset

typedef MODEL (*BASIC_MODEL_FUNC1) (const int *, DATASET *, gretlopt, PRN *);
typedef MODEL (*BASIC_MODEL_FUNC2) (const int *, DATASET *, gretlopt);


// module related tools

void module_init ()
{
    libgretl_init();
    printf("libgretl initialized\n");
}

void module_cleanup ()
{
    if (DSET != NULL) {
        destroy_dataset(DSET);
    }
    libgretl_cleanup();
    printf("libgretl cleaned up\n");
}

void about () {
    int err = 0;
    gretl_bundle *b = NULL;
    PRN *prn = NULL;

    printf("module version: %g\n", GRETL_MODULE_VERSION);
    lib_logo();
    std::cout << std::endl;

    b = get_sysinfo_bundle(&err);
    if (!err) {
        prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);
        if (prn != NULL) {
            err = gretl_bundle_print(b, prn);
        }
        if (err) {
            std::cerr << "error while printing 'sysinfo' bundle" << std::endl;
        }
    } else {
        std::cerr << "error while getting 'sysinfo' bundle" << std::endl;
    }

    if (prn != NULL) {
        gretl_print_destroy(prn);
    }
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
    set(key, std::to_string(val).c_str());
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

    gretl_dset_to_numpy_dict(fname, *res);
    return res;
}

void attach_gretl_data (const std::string fname)
{
    DATASET *dset = gretl_load_global_dataset(fname);

    if (dset != NULL) {
        destroy_dataset(DSET);
        DSET = dset;
    }
}

//Proof of concept !!!
std::string sysinfo (void)
{
    return bundle_test();
}

// model estimation

// Apparatus to handle named optional arguments, including 'print'
// and 'data' as well as gretl option flags and their parameters.

static gretlopt unpack_kwargs (const py::kwargs &kwargs,
                               int ci, int *do_print,
                               DATASET **pdset)
{
    int opt, opts = OPT_NONE;
    OptStatus status;
    bool got_data = false;

    for (auto &item : kwargs) {
        bool is_print = false;

        if (!py::isinstance<py::str>(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "data") {
            // special case: get hold of dict for dataset
            got_data = true;
            if (py::isinstance<py::dict>(item.second)) {
                const py::dict data = item.second.cast<py::dict>();

                *pdset = datainfo_new();
                numpy_dict_to_gretl_dset(data, *pdset);
            } else {
                throw std::runtime_error("'data' must be a dictionary");
            }
            continue;
        } if (key == "print") {
            // special case: 'print' is not a gretl option
            is_print = true;
            status = OPT_NO_PARM;
        } else if (key == "seasonal" && ci == ARMA) {
            // arma special: ignore here
            continue;
        } else {
            // should be a regular gretl option: check its validity in
            // the context of @ci
            if (strchr(key.c_str(), '_')) {
                char tmp[32];

                strcpy(tmp, key.c_str());
                gretl_charsub(tmp, '_', '-');
                opt = valid_long_opt(ci, tmp, &status);
            } else {
                opt = valid_long_opt(ci, key.c_str(), &status);
            }
            if (opt == 0) {
                throw std::runtime_error(std::string("invalid option flag ") + key);
            }
        }
        if (status == OPT_NO_PARM) {
            // no parameter wanted, but we'll accept a boolean
            if (py::isinstance<py::int_>(item.second)) {
                int ival = item.second.cast<int>();

                if (ival == 1) {
                    /* activate the option */
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
            // an option parameter is accepted, perhaps required
            if (py::isinstance<py::str>(item.second)) {
                const std::string &sval = item.second.cast<std::string>();
                push_option_param(ci, (gretlopt) opt, gretl_strdup(sval.c_str()));
             } else if (py::isinstance<py::int_>(item.second)) {
                int ival = item.second.cast<int>();
                push_option_param(ci, (gretlopt) opt, gretl_strdup(std::to_string(ival).c_str()));
            } else if (py::isinstance<py::float_>(item.second)) {
                double dval = item.second.cast<double>();
                push_option_param(ci, (gretlopt) opt, gretl_strdup(std::to_string(dval).c_str()));
            } else {
                throw std::runtime_error("unsupported option param type");
            }
        }
    }

    if (!got_data) {
        // if we didn't get a 'data' specification, try for the global dataset
        *pdset = DSET;
    }

    return (gretlopt) opts;
}

void finalize_model (MODEL *model, DATASET *dset,
                     int *list, int do_print,
                     PRN *prn, py::dict *res)
{
    if (model->errcode) {
        const char *msg = errmsg_get_with_default(model->errcode);
        throw std::runtime_error(msg);
    } else {
        char *buf;

        printmodel(model, dset, OPT_NONE, prn);
        buf = gretl_print_steal_buffer(prn);
        if (do_print) {
            std::fputs(buf, stdout);
        }
        gretl_model_to_dict(model, dset, buf, *res);
    }

    if (dset != DSET) {
        destroy_dataset(dset);
    }

    free(list);
    gretl_model_free(model);
    gretl_print_destroy(prn);
}

static void get_arma_order (const py::tuple &tup,
                            int *p, int *d, int *q)
{
    int n = py::len(tup);

    if (n == 2) {
        *p = tup[0].cast<int>();
        *q = tup[1].cast<int>();
    } else if (n == 3) {
        *p = tup[0].cast<int>();
        *d = tup[1].cast<int>();
        *q = tup[2].cast<int>();
    } else {
        throw std::runtime_error(std::string("order must be of length 2 or 3"));
    }
}

// arma estimation

py::dict *arma (const std::string y, const py::tuple &tup,
                const py::kwargs &kwargs)
{
    py::dict *res = new py::dict;
    DATASET *dset = NULL;
    MODEL *model;
    gretlopt opt = OPT_NONE;
    PRN *prn = NULL;
    int y_number;
    int *list;
    int p, d, q;
    int P, D, Q;
    int do_print = 0;
    int seasonal = 0;
    int i;

    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs(kwargs, ARMA, &do_print, &dset);
    } else {
        dset = DSET;
    }
    if (dset == NULL) {
        delete res;
        throw std::runtime_error("No dataset is available");
    }

    y_number = current_series_index(dset, y.c_str());
    if (y_number == -1) {
        delete res;
        throw std::runtime_error("There is no variable: " + y);
    }

    p = d = q = 0;
    P = D = Q = 0;
    get_arma_order(tup, &p, &d, &q);

    if (kwargs.contains("seasonal") && !kwargs["seasonal"].is_none()) {
        if (py::isinstance<py::tuple>(kwargs["seasonal"])) {
            const py::tuple seas = kwargs["seasonal"].cast<py::tuple>();

            get_arma_order(seas, &P, &D, &Q);
            if (P > 0 || D > 0 || Q > 0) {
                seasonal = 1;
            }
        }
    }

    model = gretl_model_new();
    list = gretl_list_new(seasonal? 9 : 5);
    prn = gretl_print_new(GRETL_PRINT_BUFFER, NULL);

    if (model == NULL || list == NULL || prn == NULL) {
        throw std::runtime_error("Failed to allocate memory");
    }

    i = 1;
    list[i++] = p;
    list[i++] = d;
    list[i++] = q;
    if (seasonal) {
        list[i++] = LISTSEP;
        list[i++] = P;
        list[i++] = D;
        list[i++] = Q;
    }
    list[i++] = LISTSEP;
    list[i++] = y_number;

    *model = arma(list, NULL, dset, opt, prn);
    finalize_model(model, dset, list, do_print, prn, res);

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

// Function to support several estimation methods that have
// the same "basic" signature, plus a few special cases

static py::dict *regression_wrapper (const std::string formula,
                                     const py::kwargs &kwargs,
                                     GretlCmdIndex ci,
                                     BASIC_MODEL_FUNC1 bmf1,
                                     BASIC_MODEL_FUNC2 bmf2)
{
    py::dict *res = new py::dict;
    DATASET *dset = NULL;
    MODEL *model = NULL;
    PRN *prn = NULL;
    int *list = NULL;
    gretlopt opt = OPT_NONE;
    int do_print = 0;

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
    prn = gretl_print_new(GRETL_PRINT_BUFFER, NULL);

    if (list == NULL || model == NULL || prn == NULL) {
        throw std::runtime_error("Failed to allocate memory");
    }

    if (ci == OLS || ci == WLS) {
        *model = lsq(list, dset, ci, opt);
    } else if (ci == LOGIT || ci == PROBIT) {
        *model = logit_probit(list, dset, ci, opt, prn);
    } else if (ci == POISSON || ci == NEGBIN) {
        *model = count_model(list, ci, dset, opt, prn);
    } else if (bmf1 != NULL) {
        *model = (*bmf1)(list, dset, opt, prn);
    } else if (bmf2 != NULL) {
        *model = (*bmf2)(list, dset, opt);
    } else {
        throw std::runtime_error("Unsupported estimator");
    }

    finalize_model(model, dset, list, do_print, prn, res);

    return res;
}

// thin wrappers for specific estimators

py::dict *ols (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, OLS, NULL, NULL);
}

py::dict *wls (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, WLS, NULL, NULL);
}

py::dict *logit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, LOGIT, NULL, NULL);
}

py::dict *probit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, PROBIT, NULL, NULL);
}

py::dict *poisson (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, POISSON, NULL, NULL);
}

py::dict *negbin (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, NEGBIN, NULL, NULL);
}

// content from automation/more_calls.cpp

py::dict *biprobit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, BIPROBIT, &biprobit_model, NULL);
}

py::dict *duration (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, DURATION, &duration_model, NULL);
}

py::dict *heckit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, HECKIT, &heckit_model, NULL);
}

py::dict *logistic (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, LOGISTIC, NULL, &logistic_driver);
}

py::dict *ar (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, AR, &ar_model, NULL);
}

py::dict *ar1 (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, AR1, &ar1_model, NULL);
}

py::dict *lad (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, LAD, NULL, &lad_model);
}

py::dict *garch (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, GARCH, &garch, NULL);
}

py::dict *mpols (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, MPOLS, NULL, &mp_ols);
}

py::dict *panel (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, PANEL, &panel_model, NULL);
}

py::dict *tsls (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper(formula, kwargs, IVREG, NULL, &ivreg);
}
