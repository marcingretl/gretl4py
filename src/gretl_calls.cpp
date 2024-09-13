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

static DATASET *DSET = NULL; // global dataset
static int gretl4py_verbose = 0;

typedef MODEL (*BASIC_MODEL_FUNC1) (const int *, DATASET *, gretlopt, PRN *);
typedef MODEL (*BASIC_MODEL_FUNC2) (const int *, DATASET *, gretlopt);


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
    if (DSET != NULL) {
        destroy_dataset(DSET);
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

static void handle_gretl_error (int err)
{
    const char *msg = errmsg_get_with_default(err);
    throw std::runtime_error(msg);
}

// set a string-valued "setvar"

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
    PRN *prn = NULL;
    int err = 0;

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
            handle_gretl_error(err);
        }
    }
}

void printdata (const std::optional<py::dict> &data,
                bool namesonly)
{
    DATASET *dset = NULL;

    if (data == std::nullopt && DSET == NULL) {
        throw std::runtime_error("No dataset in place");
    } else if (data == std::nullopt && DSET != NULL) {
        dset = DSET;
    } else {
        dset = datainfo_new();
        numpy_dict_to_gretl_dset(*data, dset);
    }

    do_printdata(dset, NULL, NULL, OPT_O, namesonly);

    if (data != std::nullopt) {
        destroy_dataset(dset);
    }
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

// Sampling of gretl datasets. As of 2024-09-02 this just covers basic usage of
// the "smpl" command in gretl: setting of t1 and/or t2; sub-sampling by means of
// a boolean condition expressed in hansl (with or without the --replace option);
// and resetting the sample to include the entire dataset ("smpl full").
// Further functionality could be added if we reckon it's worthwhile.

void sample (const py::kwargs &kwargs)
{
    char *t1str = NULL;
    char *t2str = NULL;
    char *restr = NULL;
    const char *val;
    PRN *prn = NULL;
    int opt_replace = 0;
    int full = 0;
    int argc = 0;
    int optc = 0;
    int err = 0;

    if (DSET == NULL) {
        throw std::runtime_error("No gretl dataset in place");
    }

    for (auto &item : kwargs) {
        val = NULL;

        if (!is_pystr(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (is_pyint(item.second) || is_pybool(item.second)) {
            val = std::to_string(item.second.cast<int>()).c_str();
        } else if (is_pystr(item.second)) {
            val = item.second.cast<std::string_view>().data(); /* workaround for differences between compilers and/or OSs, see commit be4b711cc8a97e03317368431e4a42f5636daa63 (be4b711) */
/* #if defined(__linux__) && defined(__clang__)
            val = std::string(py::str(item.second)).c_str();
#else
            val = item.second.cast<std::string>().c_str();
#endif // defined */
        }
        if (key == "t1") {
            t1str = gretl_strdup(val);
            argc++;
        } else if (key == "t2") {
            t2str = gretl_strdup(val);
            argc++;
        } else if (key == "restriction") {
            restr = gretl_strdup(val);
            argc++;
        } else if (key == "replace") {
            opt_replace = std::atoi(val);
            optc++;
        } else if (key == "full") {
            full = std::atoi(val);
            optc++;
        } else {
            throw std::runtime_error("invalid key '" + key + "' in 'sample' args");
        }
    }

    prn = gretl_print_new(GRETL_PRINT_STDOUT, NULL);

    // reject invalid cases
    if (full && (t1str || t2str || opt_replace)) {
        throw std::runtime_error("invalid use of 'full' in 'sample' args");
    } else if (restr && (t1str || t2str)) {
        throw std::runtime_error("invalid use of 'restriction' in 'sample' args");
    } else if (opt_replace && !restr) {
        throw std::runtime_error("invalid use of 'replace' in 'sample' args");
    }

    if (argc == 0 && optc == 0) {
        // just report the current sample
         print_smpl(DSET, get_full_length_n(), OPT_NONE, prn);
    } else if (argc == 0 && full) {
        // "smpl full"
        err = restore_full_sample(DSET, NULL);
    } else if (restr) {
        // smpl <restr> --restrict [--replace]
        gretlopt opt = OPT_R;

        if (opt_replace) {
            // C++ !!
            opt = (gretlopt) ((int) OPT_R | (int) OPT_P);
        }
        err = restrict_sample(restr, NULL, DSET, NULL, opt, prn, NULL);
    } else if (t1str || t2str) {
        // smpl <t1str> <t2str>
        if (t1str && !t2str) {
            err = set_sample(t1str, ";", DSET, OPT_NONE);
        } else if (t2str && !t1str) {
            err = set_sample(";", t2str, DSET, OPT_NONE);
        } else {
            err = set_sample(t1str, t2str, DSET, OPT_NONE);
        }
    }

    if (err) {
        const char *msg = errmsg_get_with_default(err);
        throw std::runtime_error(msg);
    } else if (argc > 0 || optc > 0) {
        print_smpl(DSET, get_full_length_n(), OPT_NONE, prn);
    }

    gretl_print_destroy(prn);

    free(t1str);
    free(t2str);
    free(restr);
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

        if (!is_pystr(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "data") {
            // special case: get hold of dict for dataset
            got_data = true;
            if (is_pydict(item.second)) {
                const py::dict data = item.second.cast<py::dict>();

                *pdset = datainfo_new();
                numpy_dict_to_gretl_dset(data, *pdset);
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
        const char *summary;
        int err;

        printmodel(model, dset, OPT_NONE, prn);
        summary = gretl_print_get_buffer(prn);
        if (do_print) {
            std::fputs(summary, stdout);
        }
        err = gretl_model_to_dict(model, dset, summary, *res);
        if (err) {
            handle_gretl_error(err);
        }
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
