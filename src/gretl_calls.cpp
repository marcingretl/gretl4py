#include "gretl4py.h" /* MUST be at the first position!!! */
#include "gretl4py_data.h"
#include <gretl/gretl_drivers.h>
#include <gretl/gretl_errors.h>
#include <gretl/libset.h>
#include <iostream>
#include <cstdlib>

#if defined(_WIN32)
# include <gretl/gretl_win32.h>
#endif

#if (defined __GNUC__) && !(defined __clang__)
# define COMPILER_IDENT "GCC " __VERSION__
#else
# define COMPILER_IDENT __VERSION__
#endif

#define LIBGRETL_MIN_VER 48
#define GRETL_PYBIND11_DEBUG 0

static GretlDataset *DSET = nullptr; // global dataset (class)
static int gretl4py_verbose = 0;


// module related tools

void module_init (void)
{
    if (libgretl_version() < LIBGRETL_MIN_VER) {
        throw std::runtime_error("gretl4py requires libgretl-1.0 version "
                                 + std::to_string(LIBGRETL_MIN_VER) + " or higher");
    }
    char *str = std::getenv("GRETL4PY_VERBOSE");
    if (str != nullptr) {
        gretl4py_verbose = std::atoi(str);
    }
    libgretl_init();
#if defined(_WIN32)
    win32_cli_read_rc();
#else
    cli_read_rc();
#endif
    if (gretl4py_verbose) {
        std::printf("libgretl initialized\n");
    }
}

void module_cleanup (void)
{
    if (DSET) {
        delete DSET;
    }
    libgretl_cleanup();
    if (gretl4py_verbose) {
        std::printf("libgretl cleaned up\n");
    }
}

py::dict about (bool do_print)
{
    std::string mod_ver, lg_ver, py11_ver;
    py::dict res;
    GretlBundle b = GretlBundle();

    gretl_push_c_numeric_locale();

    mod_ver = std::to_string(GRETL_MODULE_VERSION).substr(0,3);
    py11_ver = std::to_string(PYBIND11_VERSION_MAJOR);
    py11_ver += "." + std::to_string(PYBIND11_VERSION_MINOR);
    py11_ver += "." + std::to_string(PYBIND11_VERSION_PATCH);
    lg_ver = std::to_string(libgretl_version());

    res[py::cast("mod_ver")] = mod_ver;
    res[py::cast("py11_ver")] = py11_ver;
    res[py::cast("lg_ver")] = lg_ver;
    if (!b.err) {
        res[py::cast("sysinfo")] = b.to_json();
    } else {
        res[py::cast("sysinfo")] = py::none();
    }

    if (do_print || gretl4py_verbose) {
        std::cout << "gretl4py version " << mod_ver;
        std::cout << " [" << COMPILER_IDENT << "]";
        std::cout << ", pybind11 version " << py11_ver;
        std::cout << ", libgretl-1.0 version " << lg_ver << std::endl;
        if (!b.err) {
            std::cout << std::endl << "libgretl $sysinfo ";
            b.print();
        }
    }

    gretl_pop_c_numeric_locale();

    if (b.err) {
        std::cerr << errmsg_get_with_default(b.err) << std::endl;
    }

    return res;
}

// set a string or int valued "setvar"

void set (const std::string key,
          const std::optional<py::object> &pyval)
{
    int err = 0;

    if (pyval == std::nullopt) {
        PRN *prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);
        err = execute_set(key.c_str(), NULL, NULL, OPT_NONE, prn);
        gretl_print_destroy(prn);
    } else {
        err = execute_set(key.c_str(), MapToPy(pyval).to_string().c_str(),
                          NULL, OPT_NONE, NULL);
    }

    if (err) {
        handle_gretl_error(err);
    }
}

// data management

void attach_data (const py::dict &pydict)
{
    if (DSET) {
        delete DSET;
    }
    DSET = new GretlDataset(pydict);
}

void attach_gretl_data (const std::string fname)
{
    if (DSET) {
        delete DSET;
        DSET = nullptr; // this is because of SIGSEGV by free_Z
    }
    DSET = new GretlDataset(fname);
}

py::dict *get_gretl_data (const std::string fname)
{
    return GretlDataset(fname).to_dict();
}

void printdata (const std::optional<py::dict> &data,
                bool namesonly)
{
    if (data == std::nullopt && !DSET) {
        throw std::runtime_error("No gretl dataset in place");
    } else if (data == std::nullopt && DSET) {
        DSET->printdset(namesonly);
    } else {
        GretlDataset(*data).printdset(namesonly);
    }
}

// Sampling of gretl datasets. As of 2024-09-02 this just covers basic usage of
// the "smpl" command in gretl: setting of t1 and/or t2; sub-sampling by means of
// a boolean condition expressed in hansl (with or without the --replace option);
// and resetting the sample to include the entire dataset ("smpl full").
// Further functionality could be added if we reckon it's worthwhile.

void sample (const py::kwargs &kwargs)
{
    if (!DSET) {
        throw std::runtime_error("No gretl dataset in place");
    }
    DSET->sample(kwargs);
}

// model estimation

// Apparatus to handle named optional arguments, including 'print'
// and 'data' as well as gretl option flags and their parameters.

static gretlopt unpack_kwargs (const py::kwargs &kwargs,
                               int ci, int *do_print,
                               GretlDataset *&pdset)
{
    int opt, opts = OPT_NONE;
    OptStatus status;
    bool got_data = false;

    for (auto &item : kwargs) {
        bool is_print = false;

        if (!is_pystr(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "data") {
            // special case: get hold of dict for dataset
            got_data = true;
            if (is_pydict(item.second)) {
                const py::dict data = item.second.cast<py::dict>();
                pdset = new GretlDataset(data);
#if GRETL_PYBIND11_DEBUG
                std::printf("we have the following variables in given dataset:\n");
                pdset->printdset(true);
#endif // GRETL_PYBIND11_DEBUG
            } else {
                throw std::runtime_error("'data' must be a dictionary");
            }
            continue;
        } else if (key == "print") {
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
                throw std::runtime_error("invalid option flag " + key);
            }
        }
        if (status == OPT_NO_PARM) {
            // no parameter wanted, but we'll accept a boolean
            if (is_pyint(item.second)) {
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
            if (is_pystr(item.second)) {
                const std::string &sval = item.second.cast<std::string>();
                push_option_param(ci, (gretlopt) opt, gretl_strdup(sval.c_str()));
            } else if (is_pyint(item.second)) {
                int ival = item.second.cast<int>();
                push_option_param(ci, (gretlopt) opt, gretl_strdup(std::to_string(ival).c_str()));
            } else if (is_pyfloat(item.second)) {
                double dval = item.second.cast<double>();
                push_option_param(ci, (gretlopt) opt, gretl_strdup(std::to_string(dval).c_str()));
            } else {
                throw std::runtime_error("unsupported option param type");
            }
        }
    }

    if (!got_data) {
        // if we didn't get a 'data' specification, try for the global dataset
#if GRETL_PYBIND11_DEBUG
        std::printf("unpack_kwargs: we try for the global dataset\n");
#endif // GRETL_PYBIND11_DEBUG
        pdset = DSET;
    }

    return (gretlopt) opts;
}

void finalize_model (MODEL *model, DATASET *dset,
                     int *list, int do_print,
                     PRN *prn, py::dict &res)
{
    if (model->errcode) {
        const char *msg = errmsg_get_with_default(model->errcode);
        throw std::runtime_error(msg);
    } else {
        const char *summary;
        int err;

        printmodel(model, dset, OPT_NONE, prn);
        summary = gretl_print_get_buffer(prn);
        if (do_print) {
            std::fputs(summary, stdout);
        }
        err = gretl_model_to_dict(model, dset, summary, res);
        if (err) {
            handle_gretl_error(err);
        }
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
    GretlDataset *GD = nullptr;
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
        opt = unpack_kwargs(kwargs, ARMA, &do_print, GD);
    } else {
        GD = DSET;
    }
    if (!GD) {
        delete res;
        throw std::runtime_error("No dataset is available");
    }

    y_number = current_series_index(GD->dset, y.c_str());
    if (y_number == -1) {
        delete res;
        throw std::runtime_error("There is no variable: " + y);
    }

    p = d = q = 0;
    P = D = Q = 0;
    get_arma_order(tup, &p, &d, &q);

    if (kwargs.contains("seasonal") && !kwargs["seasonal"].is_none()) {
        if (is_pytuple(kwargs["seasonal"])) {
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

    *model = arma(list, NULL, GD->dset, opt, prn);
    finalize_model(model, GD->dset, list, do_print, prn, *res);

    if (GD != DSET) {
        delete GD;
    }

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
// F: function to call
// CI: GretlCmdIndex
// T: F signature type (argument list)
//  -> 0: list, dset, CI, opt (OLS || WLS)
//  -> 1: list, dset, CI, opt, prn (LOGIT || PROBIT)
//  -> 2: list, CI, dset, opt, prn (POISSON || NEGBIN)
//  -> 3: list, dset, opt, prn
//  -> 4: list, dset, opt

template <auto F, GretlCmdIndex CI, int T>
static py::dict *regression_wrapper (const std::string &formula,
                                     const py::kwargs &kwargs)
{
    py::dict *res = new py::dict;
    GretlDataset *GD = nullptr;
    MODEL *model = NULL;
    PRN *prn = NULL;
    int *list = NULL;
    gretlopt opt = OPT_NONE;
    int do_print = 0;

    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs(kwargs, CI, &do_print, GD);
    } else {
        GD = DSET;
    }
    if (!GD) {
        throw std::runtime_error("No dataset is available");
    }

    list = list_from_formula(formula.c_str(), GD->dset);
    model = gretl_model_new();
    prn = gretl_print_new(GRETL_PRINT_BUFFER, NULL);

    if (list == NULL || model == NULL || prn == NULL) {
        throw std::runtime_error("Failed to allocate memory");
    }

    if constexpr (T == 0) {
        *model = F(list, GD->dset, CI, opt);
    } else if constexpr (T == 1) {
        *model = F(list, GD->dset, CI, opt, prn);
    } else if constexpr (T == 2) {
        *model = F(list, CI, GD->dset, opt, prn);
    } else if constexpr (T == 3 ) {
        *model = F(list, GD->dset, opt, prn);
    } else if constexpr (T == 4) {
        *model = F(list, GD->dset, opt);
    }

    finalize_model(model, GD->dset, list, do_print, prn, *res);

    if (GD != DSET) {
        delete GD;
    }

    return res;
}

// thin wrappers for specific estimators

py::dict *ols (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<lsq, OLS, 0>(formula, kwargs);
}

py::dict *wls (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<lsq, WLS, 0>(formula, kwargs);
}

py::dict *logit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<logit_probit, LOGIT, 1>(formula, kwargs);
}

py::dict *probit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<logit_probit, PROBIT, 1>(formula, kwargs);
}

py::dict *poisson (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<count_model, POISSON, 2>(formula, kwargs);
}

py::dict *negbin (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<count_model, NEGBIN, 2>(formula, kwargs);
}

// content from automation/more_calls.cpp

py::dict *biprobit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<biprobit_model, BIPROBIT, 3>(formula, kwargs);
}

py::dict *duration (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<duration_model, DURATION, 3>(formula, kwargs);
}

py::dict *heckit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<heckit_model, HECKIT, 3>(formula, kwargs);
}

py::dict *logistic (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<logistic_driver, LOGISTIC, 4>(formula, kwargs);
}

py::dict *ar (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<ar_model, AR, 3>(formula, kwargs);
}

py::dict *ar1 (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<ar1_model, AR1, 3>(formula, kwargs);
}

py::dict *lad (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<lad_model, LAD, 4>(formula, kwargs);
}
/*
py::dict *garch (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<garch, 3>(formula, kwargs, GARCH);
}*/

py::dict *mpols (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<mp_ols, MPOLS, 4>(formula, kwargs);
}

py::dict *panel (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<panel_model, PANEL, 3>(formula, kwargs);
}

py::dict *tsls (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<ivreg, IVREG, 4>(formula, kwargs);
}
