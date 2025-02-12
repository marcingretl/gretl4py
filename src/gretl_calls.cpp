/*
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "gretl4py.h" /* MUST be at the first position!!! */
#include "gretl4py_data.h"
#include <gretl/gretl_drivers.h>
#include <gretl/gretl_errors.h>
#include <gretl/libset.h>
extern "C" {
    #include <gretl/gretl_string_table.h>
}
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

#define LIBGRETL_MIN_VER 49
#define GRETL_PYBIND11_DEBUG 0

#define use_ldco_sig(c) (c == OLS || c == WLS) //const int *list, DATASET *dset, GretlCmdIndex ci, gretlopt opt
#define use_ldcop_sig(c) (c == LOGIT || c == PROBIT) //int *list, DATASET *dset, int ci, gretlopt opt, PRN *prn
#define use_ldo_sig(c) (c == LOGISTIC || c == LAD \
                        || c == MPOLS || c == IVREG) //const int *list, DATASET *dset, gretlopt opt
#define use_ldop_sig(c) (c == BIPROBIT || c == DURATION \
                         || c == HECKIT || c == AR || c == AR1 \
                         || c == GARCH || c == PANEL) //const int *list, DATASET *dset, gretlopt opt, PRN *prn
#define use_lcdop_sig(c) (c == POISSON || c == NEGBIN) //const int *list, int ci, DATASET *dset, gretlopt opt, PRN *prn

static GretlDataset *DSET = nullptr; // global dataset (class)
static int gretl4py_verbose = 0;

// workaround to manually resolve overloads ambiguity
static MODEL (&garch_to_call)(const int*, DATASET*, gretlopt, PRN*) = garch;

//////////////////////////////////////////////////
// module related tools
//////////////////////////////////////////////////

void module_init (void)
{
    if (libgretl_version() < LIBGRETL_MIN_VER) {
        throw std::runtime_error("gretl4py requires libgretl-1.0 version "
                                 + std::to_string(LIBGRETL_MIN_VER) + " or higher");
    }
    char *str = std::getenv("GRETL4PY_VERBOSE");
    if (str) {
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
    GretlBundle b = GretlBundle();
    const std::string lgretl_strings[10] =
        {"dotdir", "gnuplot", "gretldir", "lang", "seats",
        "tramo", "tramodir", "workdir", "x12a", "x12adir"};
    std::string mod_ver, lg_ver, py11_ver, sj_ver, sj_name, sj_desc;
    py::dict res, built_in_strings;

    gretl_push_c_numeric_locale();

    mod_ver = std::to_string(GRETL_MODULE_VERSION).substr(0,3);
    py11_ver = std::to_string(PYBIND11_VERSION_MAJOR);
    py11_ver += "." + std::to_string(PYBIND11_VERSION_MINOR);
    py11_ver += "." + std::to_string(PYBIND11_VERSION_PATCH);
    lg_ver = std::to_string(libgretl_version());
    sj_ver = SIMDJSON_VERSION;
    sj_name = simdjson::get_active_implementation()->name();
    sj_desc = simdjson::get_active_implementation()->description();
#if USE_MAPBOX_VARIANT
    std::string mbv_ver;
    mbv_ver = std::to_string(VARIANT_MAJOR_VERSION);
    mbv_ver += "." + std::to_string(VARIANT_MINOR_VERSION);
    mbv_ver += "." + std::to_string(VARIANT_PATCH_VERSION);
#endif // USE_MAPBOX_VARIANT

    // built-in strings
    for (int i=0; i<10; i++ ) {
        built_in_strings[py::cast(lgretl_strings[i])] =
            get_built_in_string_by_name(lgretl_strings[i].c_str());
    }

    res[py::cast("mod_ver")] = mod_ver;
    res[py::cast("py11_ver")] = py11_ver;
    res[py::cast("lg_ver")] = lg_ver;
    if (!b.err) {
        py::dict *sysinfo = b.to_dict();
        res[py::cast("sysinfo")] = *sysinfo;
        delete sysinfo;
    } else {
        res[py::cast("sysinfo")] = py::none();
    }
    res[py::cast("built_in_strings")] = built_in_strings;
    res[py::cast("sj_ver")] = sj_ver;
    res[py::cast("sj_name")] = sj_name;
    res[py::cast("sj_desc")] = sj_desc;
#if USE_MAPBOX_VARIANT
    res[py::cast("mbv_ver")] = mbv_ver;
#endif // USE_MAPBOX_VARIANT

    if (do_print || gretl4py_verbose) {
        std::cout << "gretl4py v" << mod_ver;
        std::cout << " [" << COMPILER_IDENT << "]" << std::endl;
        std::cout << "\t+ pybind11     v" << py11_ver << std::endl;
        std::cout << "\t+ simdjson     v" << sj_ver << std::endl;
#if USE_MAPBOX_VARIANT
        std::cout << "\t+ mapbox       v" << mbv_ver << std::endl;
#endif // USE_MAPBOX_VARIANT
        std::cout << "\t+ libgretl-1.0 v" << lg_ver << std::endl;
        if (!b.err) {
            std::cout << std::endl << "libgretl $sysinfo:" << std::endl;
            b.print();
        }
        std::cout << "libgretl built-in strings:" << std::endl;
        for (auto item : built_in_strings) {
            std::cout << "  " << item.first << " = ";
            if (!MapPyToCpp(item.second).to_string().empty()) {
                std::cout << "\"" << item.second << "\"";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl << "simdjson architecture implementation (runtime):" << std::endl;
        std::cout << "  name = " << "\"" << sj_name << "\"" << std::endl;
        std::cout << "  description = " << "\"" << sj_desc << "\"" << std::endl;
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
    GretlPrint GP = GretlPrint();
    int err = 0;

    if (pyval == std::nullopt) {
        err = execute_set(key.c_str(), NULL, NULL, OPT_NONE, GP.prn);
    } else {
        err = execute_set(key.c_str(), MapPyToCpp(pyval).to_string().c_str(),
                          NULL, OPT_NONE, GP.prn);
    }

    GP.print();

    if (err) {
        handle_gretl_error(err);
    }
}

//////////////////////////////////////////////////
// data management
//////////////////////////////////////////////////

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

//////////////////////////////////////////////////
// model estimation
//////////////////////////////////////////////////

// Apparatus to handle named optional arguments, including 'print'
// and 'data' as well as gretl option flags and their parameters.

static gretlopt unpack_kwargs (const py::kwargs &kwargs,
                               int ci, GretlDataset *&pdset)
{
    int opt, opts = OPT_NONE;
    bool got_data = false;
    OptStatus status;

    for (auto &item : kwargs) {
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
        } else if (key == "seasonal" && ci == ARMA) {
            // arima special: ignore here
            continue;
        } else if (key == "lagselect" && ci == ARMA) {
            throw std::runtime_error("'lagselect' option for (S)ARIMA is temporarily unavailable");
        } else {
            // should be a regular gretl option: check its validity in
            // the context of @ci
            if (std::strchr(key.c_str(), '_')) {
                std::string tmp(key);

                gretl_charsub(tmp.data(), '_', '-');
                opt = valid_long_opt(ci, tmp.c_str(), &status);
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
                    opts |= opt;
                } else if (ival != 0) {
                    throw std::runtime_error("invalid option parameter");
                }
            }
        } else {
            // an option parameter is accepted, perhaps required
#if 1
            if (!is_pystr(item.second) && !is_pyint(item.second) && !is_pyfloat(item.second)) {
                throw std::runtime_error("unsupported option param type");
            } else {
                push_option_param(ci, static_cast<gretlopt>(opt),
                                  gretl_strdup(MapPyToCpp(item.second).to_string().c_str()));
            }
#else
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
#endif // 1
        }
    }

    if (!got_data) {
        // if we didn't get a 'data' specification, try for the global dataset
#if GRETL_PYBIND11_DEBUG
        std::printf("unpack_kwargs: we try for the global dataset\n");
#endif
        pdset = DSET;
    }

    return (gretlopt) opts;
}

static py::dict *save_model (MODEL *model, DATASET *dset, PRN *prn,
                             gretlopt opt = OPT_NONE)
{
    py::dict *res = nullptr;

    if (model->errcode) {
        const char *msg = errmsg_get_with_default(model->errcode);
        throw std::runtime_error(msg);
    } else {
        const char *summary;
        int err;

        printmodel(model, dset, opt, prn);
        summary = gretl_print_get_buffer(prn);
        res = gretl_model_to_dict(model, dset, summary, &err);
        if (err) {
            handle_gretl_error(err);
        }
    }

    return res;
}

static void get_arima_order (const py::tuple &tup,
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

// (s)arima estimation

py::dict *arima (const std::string y, const py::tuple &tup,
                 const py::kwargs &kwargs)
{
    GretlPrint GP = GretlPrint();
    gretlopt opt = OPT_NONE;
    py::dict *res = nullptr;
    GretlDataset *GD = nullptr;
    MODEL *model = NULL;
    int *list = NULL;
    int seasonal = 0;
    int y_number;
    int p, d, q;
    int P, D, Q;
    int i;

    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs(kwargs, ARMA, GD);
    } else {
        GD = DSET;
    }
    if (!GD) {
        throw std::runtime_error("No dataset is available");
    }

    y_number = current_series_index(GD->dset, y.c_str());
    if (y_number == -1) {
        throw std::runtime_error("There is no variable: " + y);
    }

    p = d = q = 0;
    P = D = Q = 0;
    get_arima_order(tup, &p, &d, &q);

    if (kwargs.contains("seasonal") && !kwargs["seasonal"].is_none()) {
        if (is_pytuple(kwargs["seasonal"])) {
            const py::tuple seas = kwargs["seasonal"].cast<py::tuple>();

            get_arima_order(seas, &P, &D, &Q);
            if (P > 0 || D > 0 || Q > 0) {
                seasonal = 1;
            }
        }
    }

    model = gretl_model_new();
    list = gretl_list_new(seasonal? 9 : 5);
    if (model == NULL || list == NULL || GP.prn == NULL) {
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

    *model = arma(list, NULL, GD->dset, opt, GP.prn);
    res = save_model(model, GD->dset, GP.prn, opt);

    gretl_model_free(model);
    gretl_list_free(list);

    if (GD != DSET) {
        delete GD;
    }

    return res;
}

// convert from a simple R-style formula to a gretl list

static int *list_from_formula (const char *formula,
                               DATASET *dset)
{
    char *tmp = strdup(formula);
    int *list = NULL;
    int err = 0;

    gretl_charsub(tmp, '~', ' ');
    gretl_charsub(tmp, '+', ' ');
    list = generate_list(tmp, dset, 0, &err);
    std::free(tmp);

    if (err) {
        std::cerr << "libgretl error occurred: " << err << std::endl;
        if (gretl4py_verbose) {
            std::perror(gretl_errmsg_get());
        }
    }

    return list;
}

// Function template to support several estimation methods that
// have the same "basic" signature, plus a few special cases
// F: function to call
// CI: GretlCmdIndex

template <auto F, GretlCmdIndex CI>
static py::dict *regression_wrapper (const std::string &formula,
                                     const py::kwargs &kwargs)
{
    GretlPrint GP = GretlPrint();
    gretlopt opt = OPT_NONE;
    py::dict *res = nullptr;
    GretlDataset *GD = nullptr;
    MODEL *model = NULL;
    int *list = NULL;

    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs(kwargs, CI, GD);
    } else {
        GD = DSET;
    }
    if (!GD) {
        throw std::runtime_error("No dataset is available");
    }

    list = list_from_formula(formula.c_str(), GD->dset);
    model = gretl_model_new();

    if (list == NULL) {
        throw std::runtime_error("Formula rendering failed");
    } else if (model == NULL || GP.prn == NULL) {
        throw std::runtime_error("Failed to allocate memory");
    }

    if constexpr (use_ldco_sig(CI)) {
        *model = F(list, GD->dset, CI, opt);
    } else if constexpr (use_ldcop_sig(CI)) {
        *model = F(list, GD->dset, CI, opt, GP.prn);
    } else if constexpr (use_ldo_sig(CI)) {
        *model = F(list, GD->dset, opt);
    } else if constexpr (use_ldop_sig(CI)) {
        *model = F(list, GD->dset, opt, GP.prn);
    } else if constexpr (use_lcdop_sig(CI)) {
        *model = F(list, CI, GD->dset, opt, GP.prn);
    }
    res = save_model(model, GD->dset, GP.prn, opt);

    gretl_model_free(model);
    gretl_list_free(list);

    if (GD != DSET) {
        delete GD;
    }

    return res;
}

// thin wrappers for specific estimators

py::dict *ols (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<lsq, OLS>(formula, kwargs);
}

py::dict *wls (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<lsq, WLS>(formula, kwargs);
}

py::dict *logit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<logit_probit, LOGIT>(formula, kwargs);
}

py::dict *probit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<logit_probit, PROBIT>(formula, kwargs);
}

py::dict *poisson (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<count_model, POISSON>(formula, kwargs);
}

py::dict *negbin (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<count_model, NEGBIN>(formula, kwargs);
}

// content from automation/more_calls.cpp

py::dict *biprobit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<biprobit_model, BIPROBIT>(formula, kwargs);
}

py::dict *duration (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<duration_model, DURATION>(formula, kwargs);
}

py::dict *heckit (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<heckit_model, HECKIT>(formula, kwargs);
}

py::dict *logistic (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<logistic_driver, LOGISTIC>(formula, kwargs);
}

py::dict *ar (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<ar_model, AR>(formula, kwargs);
}

py::dict *ar1 (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<ar1_model, AR1>(formula, kwargs);
}

py::dict *lad (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<lad_model, LAD>(formula, kwargs);
}

py::dict *garch (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<garch_to_call, GARCH>(formula, kwargs);
}

py::dict *mpols (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<mp_ols, MPOLS>(formula, kwargs);
}

py::dict *panel (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<panel_model, PANEL>(formula, kwargs);
}

py::dict *tsls (const std::string formula, const py::kwargs &kwargs)
{
    return regression_wrapper<ivreg, IVREG>(formula, kwargs);
}
