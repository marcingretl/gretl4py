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

#include <pybind11/pybind11.h>
#include "gretl4py_data.h"
#include "gretl4py_model.h"
#include <gretl/forecast.h>
#include <string>

#define use_ldco_sig(c) (c == OLS || c == WLS) //const int *list, DATASET *dset, GretlCmdIndex ci, gretlopt opt
#define use_ldcop_sig(c) (c == LOGIT || c == PROBIT) //int *list, DATASET *dset, int ci, gretlopt opt, PRN *prn
#define use_ldo_sig(c) (c == LOGISTIC || c == LAD \
                        || c == MPOLS || c == IVREG) //const int *list, DATASET *dset, gretlopt opt
#define use_ldop_sig(c) (c == BIPROBIT || c == DURATION \
                         || c == HECKIT || c == AR || c == AR1 \
                         || c == GARCH || c == PANEL) //const int *list, DATASET *dset, gretlopt opt, PRN *prn
#define use_lcdop_sig(c) (c == POISSON || c == NEGBIN) //const int *list, int ci, DATASET *dset, gretlopt opt, PRN *prn

#define GRETL4PY_MODEL_DEBUG 0

//
// GretlModel class methods
//
GretlModel::GretlModel (GretlDataset *GD)
{
    this->id = next_model_id++;

    if (GD) {
        this->GD = GD;
    }
}

GretlModel::~GretlModel (void)
{
    if (this->GD) {
        this->GD->linked_models_del(this->id);
    }

    if (this->var_list != NULL) {
#if defined(_WIN32)
        gretl_list_free(var_list);
#else
        std::free(var_list);
#endif
    }
}

int GretlModel::get_id (void) const
{
    return id;
}

void GretlModel::unlink_from_dataset (void) const
{
    if (GD) {
        GD->linked_models_del(id);
    }
}

gretlopt GretlModel::unpack_kwargs (const py::kwargs &kwargs, GretlCmdIndex ci)
{
    int opt, opts = OPT_NONE;
    OptStatus status;

    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "data") {
            // special case: we use already allocated object of GretlDataset class
            GD = item.second.cast<GretlDataset*>();
            continue;
        } else if (key == "quiet") {
            throw std::runtime_error("invalid option flag: " + key);
        } else if (key == "seasonal" && ci == ARMA) {
            // arima special: ignore here
            continue;
        } else if (key == "lagselect" && ci == ARMA) {
            throw std::runtime_error("'lagselect' option for (S)ARIMA is temporarily unavailable");
        } else if ((key == "llimit" || key == "rlimit") && ci == TOBIT) {
            // TOBIT special: ignore here
            if (!is_pynumeric(item.second)) {
                throw std::runtime_error("both llimit and rlimit must be numeric");
            } else {
                if (key == "llimit") {
                    optional_par.at(0) = item.second.cast<double>();
                } else {
                    optional_par.at(1) = item.second.cast<double>();
                }
            }
            continue;
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
                throw std::runtime_error("invalid option flag: " + key);
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
            if (!is_pystr(item.second) && !is_pyint(item.second) && !is_pyfloat(item.second)) {
                throw std::runtime_error("unsupported option param type");
            } else {
                push_option_param(ci, static_cast<gretlopt>(opt),
                                  gretl_strdup(MapPyToCpp(item.second).to_string().c_str()));
            }
        }
    }

    return static_cast<gretlopt>(opts);
}


gretlopt GretlModel::unpack_kwargs_forecast (const py::kwargs &kwargs,
                                             GretlCmdIndex ci)
{
    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::runtime_error("option key must be a string");
        }

        std::cout << "the following pairs of forecasting options were passed:\n";
        std::cout << item.first.cast<std::string>() << " : ";
        std::cout << MapPyToCpp(item.second).to_string_view() << std::endl;
    }

    return OPT_NONE;
}


void GretlModel::unpack_kwargs_test (const std::string &type,
                                    const py::kwargs &kwargs,
                                    int &order)
{
    //
    // we setup test options
    // order: for autocorrelation or arch test (may chanche in future)
    //

    // we set basic opt for given test
    if (type == "normality") {
        opt = OPT_N;
    } else if (type == "logs") {
        opt = OPT_L;
    } else if (type == "squares") {
        opt = OPT_S;
    } else if (type == "autocorr") {
        opt = OPT_A;
    } else if (type == "arch") {
        opt = OPT_H;
    } else if (type == "white") {
        opt = OPT_W;
    } else if (type == "white-nocross") {
        opt = OPT_X;
    } else if (type == "breusch-pagan") {
        opt = OPT_B;
    } else if (type == "panel") {
        opt = OPT_P;
    } else if (type == "comfac") {
        opt = OPT_C;
    } else if (type == "xdepend") {
        opt = OPT_D;
    } else {
        throw std::runtime_error("unknown test");
    }

    // now we loop over kwargs;
    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "order") {
            //special case for autocorrelation and arch: lag order
            if (is_pyint(item.second)) {
                order = item.second.cast<int>();
                continue;
            } else {
                throw std::runtime_error("'order' takes integers only");
            }
        } else if (key == "quiet" || key == "silent") {
            throw std::runtime_error("invalid option flag: " + key);
        } else if (key == "univariate") {
            if (is_pybool(item.second)) {
                if (item.second.cast<bool>() == true) {
                    opt = static_cast<gretlopt>(opt | OPT_U);
                }
                continue;
            } else {
                throw std::runtime_error("'univariate' takes true/false only");
            }
        } else if (key == "robust") { //FIXME: Allin, is that right?
            if (is_pybool(item.second)) {
                if (item.second.cast<bool>() == true) {
                    opt = static_cast<gretlopt>(opt | OPT_R);
                }
                continue;
            } else {
                throw std::runtime_error("'robust' takes true/false only");
            }
        } else {
            throw std::runtime_error("invalid option parameter");
        }
    }
}


//
// Native Single Equation libgretl MODEL (NSE)
// extends: GretlModel
//
GretlModel_NSE::GretlModel_NSE (GretlDataset *GD)
                                        : GretlModel(GD) // explicitly call base class constructor!
{
#if GRETL4PY_MODEL_DEBUG
    std::cout << "GretlModel_NSE, id = " << this->get_id();
    std::cout << ", GD  = " << GD << std::endl;
#endif // GRETL4PY_MODEL_DEBUG
}

GretlModel_NSE::~GretlModel_NSE (void)
{
    if (this->model != NULL && this->model_free) {
        gretl_model_free(this->model);
    }
}

void GretlModel_NSE::save_model (gretlopt opt)
{
    //
    // Here we construct a python dictionary to represent a gretl
    // MODEL, using as intermediate step the gretl_bundle produced by
    // libgretl's bundle_from_model(). We also add the @summary text
    // produced by libgretl's printmodel().
    //
    if (model->errcode) {
        const char *msg = errmsg_get_with_default(model->errcode);
        throw std::runtime_error(msg);
    } else {
        printmodel(model, GD->dset, opt, GP.prn_ptr());
        GretlBundle b = GretlBundle(model, GD->dset);
        err = b.err;

        if (!err) {
            RES = b.to_dict();
            if (!RES) {
                err = E_DATA;
                throw std::runtime_error("Model bundle to dict conversion failed.");
            } else {
                py::dict _ids;
                _ids[py::cast("_id")] = model->ID;
                _ids[py::cast("_model_id")] = id;
                (*RES)[py::cast("_ids")] = _ids;
                (*RES)[py::cast("summary")] = GP.get_buffer();
            }
        }
        if (err) {
            handle_gretl_error(err);
        } else {
            // we link model with dataset used
            GD->linked_models_add(id);
        }
    }
}


py::dict *GretlModel_NSE::forecast (const py::kwargs &kwargs)
{
    /* this code was rendered from gui/library.c */
    FITRESID *fr = NULL;
    gretlopt gopt;
    gretlopt opt = OPT_NONE;                //FIXME: change name, must be local!!!
    FcastFlags flags = static_cast<FcastFlags>(0);
    int t2, t1 = 0;
    int premax, pre_n = 0;
    int t1min = 0;
    int recursive = 0, k = 1, *kptr;        //FIXME: recursive as bool
    int dt2 = GD->dset->n - 1;
    int st2 = GD->dset->n - 1;
    double conf = 0.95;
    //??
    char startobs[OBSLEN], endobs[OBSLEN];

    // @Allin: please verify: BEGIN
    // we do some sample problem
    err = model_sample_problem(model, GD->dset);
    if (err) {
        handle_gretl_error(err);
        throw std::runtime_error("model sample problem");
    }

    // try to figure which options might be applicable
    forecast_options_for_model(model, GD->dset, &flags, &dt2, &st2);

    if (flags & (FC_DYNAMIC_OK | FC_AUTO_OK)) {
        t2 = dt2;
    } else {
        t2 = st2;
    }

    // if no out-of-sample obs are available in case of time-series data, alert the user
    /*
    if (t2 <= model->t2 && dataset_is_time_series(GD->dset)) {
        err = out_of_sample_info(flags & FC_ADDOBS_OK, &t2);
        if (err) {
            throw std::runtime_error("no out-of-sample obs are available");
        }
    }
    */

    // max number of pre-forecast obs in "best case"
    premax = GD->dset->n - 1;

    // if there are spare obs available, default to an out-of-sample forecast
    if (t2 > model->t2) {
        t1 = model->t2 + 1;
        pre_n = model->t2 / 2;
        if (pre_n > 100) {
            pre_n = 100;
        }
        if (model->ci == GARCH) {
            /* force out-of-sample fcast */
            t1min = t1;
        }
    } else {
        pre_n = 0;
    }

    if (flags & FC_INTEGRATE_OK) {
        kptr = NULL;
    } else {
        kptr = &k;
    }

    // setting options
    unpack_kwargs_forecast(kwargs, static_cast<GretlCmdIndex>(model->ci));
    opt = OPT_S;                //FIXME: unpack_kwargs_forecast!!!
    gopt = OPT_M;               //FIXME: unpack_kwargs_forecast!!!

    if (gopt & OPT_I) {
        // transfer OPT_I (integrate forecast) from graph to general options
        opt = static_cast<gretlopt>(opt | OPT_I);
        gopt = static_cast<gretlopt>(gopt & ~OPT_I);
    }

    if (gopt & OPT_M) {
        // OPT_M (show interval for mean): copy to opt
        opt = static_cast<gretlopt>(opt | OPT_M);
    }

    // let's forecast
    if (recursive) {
        fr = recursive_OLS_k_step_fcast(model, GD->dset,
                                        t1, t2, k, pre_n, &err);
    } else {
        ntolabel(startobs, t1, GD->dset);
        ntolabel(endobs, t2, GD->dset);
        /*lib_command_sprintf("fcast %s %s%s", startobs, endobs, print_flags(opt, FCAST));
        if (parse_lib_command()) {
            throw std::runtime_error("parsing command failed");
        }*/
        fr = get_forecast(model, t1, t2, pre_n, GD->dset, opt, &err);
        /*
        if (!err) {
            record_lib_command();
        }*/
    }

    if (err) {
        handle_gretl_error(err);
    } else {
        //err = bufopen(&fp.prn);
        std::cerr << "bufopen() unavailable\n";
    }

    if (!err) {
        int ols_special = 0;
        int width = 78;

        if (recursive) {
            err = text_print_fit_resid(fr, GD->dset, GP.prn_ptr());
        } else {
            if (dataset_is_cross_section(GD->dset)) {
                ols_special = gretl_is_simple_OLS(model);
            }
            if (LIMDEP(model->ci) || ols_special) {
                // don't generate plot via text_print_forecast()
                gopt = static_cast<gretlopt>(gopt & ~OPT_P);
            } else {
                gopt = static_cast<gretlopt>(gopt | OPT_P);
            }
            fr->alpha = 1 - conf;
            err = text_print_forecast(fr, GD->dset, gopt, GP.prn_ptr());
        }

        if (ols_special) {
            err = plot_simple_fcast_bands(model, fr, GD->dset, gopt);
            gopt = static_cast<gretlopt>(gopt | OPT_P);
        }
        /*
        if (!err && (gopt & OPT_P)) {
            register_graph();
        }
        */
        if (!recursive && fr->sderr == NULL) {
            width = 60;
        }
    }

    // don't remember the "mean" option
    gopt = static_cast<gretlopt>(gopt & ~OPT_M);

    if (fr != NULL) {
        free_fit_resid(fr);
    }

    GP.print();     //FIXME: we use here instance-wide printer, but if that is not safe let's use GretlPrint fp = GretlPrint();
    // @Allin: please verify: END

    return RES_fcast;
}


GretlTypes *GretlModel_NSE::get_accessor (const std::string &accessor) const
{
    RES_other = gretl_generate_gretl_object(accessor.c_str(),
                                            err, GRETL_OBJ_EQN,
                                            GD->dset, model);
    if (err) {
        handle_gretl_error(err);
        throw std::runtime_error("grabbing '" + accessor + "' failed");
    }

    return RES_other;
}

py::dict GretlModel_NSE::test (const std::string &type,
                               const py::kwargs &kwargs)
{
    //
    // this method mimics 'modtest' command
    //
    py::dict RES;
    int order = GD->dset->pd;

    // we set this model as the last in libgretl
    set_as_last_model(model, GRETL_OBJ_EQN);
    if (model_free) {   // as of 2025-04-12: double free
        model_free = false;
    }

    // we set up options
    opt = OPT_NONE;
    unpack_kwargs_test(type, kwargs, order);

    // we perform the test
    err = model_test_driver(order, GD->dset, opt, GP.prn_ptr());

    if (err) {
        handle_gretl_error(err);
    } else {
        RES_other = get_accessor("$test");
        RES[py::cast("test")] = *RES_other;
        delete RES_other;

        RES_other = get_accessor("$pvalue");
        RES[py::cast("pvalue")] = *RES_other;
        delete RES_other;

        RES[py::cast("summary")] = GP.get_buffer();
    }

    return RES;
}


//
// NSE via dedicated Driver
//
template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver<F, CI>::GretlModel_NSE_Driver (GretlDataset *GD)
                                        : GretlModel_NSE(GD)  // explicitly call base class constructor!
{
    if constexpr (CI == TOBIT) {
        optional_par.resize(2);
        optional_par.at(0) = NADBL;
        optional_par.at(1) = NADBL;
    }
#if GRETL4PY_MODEL_DEBUG
    std::cout << "GretlModel_NSE_Driver, id = " << this->get_id();
    std::cout << ", GD  = " << GD << std::endl;
#endif // GRETL4PY_MODEL_DEBUG
}

template <auto F, GretlCmdIndex CI>
void GretlModel_NSE_Driver<F, CI>::list_from_formula (const std::string &formula)
{
    // convert from a simple R-style formula to a gretl list
    char *tmp = strdup(formula.c_str());

    gretl_charsub(tmp, '~', ' ');
    gretl_charsub(tmp, '+', ' ');
    var_list = generate_list(tmp, GD->dset, 0, &err);
    std::free(tmp);

    if (err) {
        std::cerr << "libgretl error occurred: " << err << std::endl;
        //FIXME: if (gretl4py_verbose) { std::perror(gretl_errmsg_get()); }
    }
}

// Method template to support several estimation methods that
// have the same "basic" signature, plus a few special cases
// F: function to call
// CI: GretlCmdIndex
template <auto F, GretlCmdIndex CI>
py::dict *GretlModel_NSE_Driver<F, CI>::fit (const std::string &formula,
                                             const py::kwargs &kwargs)
{
    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs(kwargs, CI);
    }

    if (!GD) {
        throw std::runtime_error("No dataset is available");
    }

    // we construct the model
    list_from_formula(formula);
    model = gretl_model_new();

    if (var_list == NULL) {
        throw std::runtime_error("Formula rendering failed");
    } else if (model == NULL || GP.prn_ptr() == NULL) {
        throw std::runtime_error("Failed to allocate memory");
    }

    // we call F as an estimator
    if constexpr (use_ldco_sig(CI)) {
        *model = F(var_list, GD->dset, CI, opt);
    } else if constexpr (use_ldcop_sig(CI)) {
        *model = F(var_list, GD->dset, CI, opt, GP.prn_ptr());
    } else if constexpr (use_ldo_sig(CI)) {
        *model = F(var_list, GD->dset, opt);
    } else if constexpr (use_ldop_sig(CI)) {
        *model = F(var_list, GD->dset, opt, GP.prn_ptr());
    } else if constexpr (use_lcdop_sig(CI)) {
        *model = F(var_list, CI, GD->dset, opt, GP.prn_ptr());
    } else if constexpr (CI == QUANTREG) {
        if (this->optional_par_as_mat != NULL) {
            *model = quantreg (this->optional_par_as_mat,
                               var_list, GD->dset, opt, GP.prn_ptr());
        } else {
            *model = F(std::to_string(this->optional_par.at(0)).c_str(),
                       var_list, GD->dset, opt, GP.prn_ptr());
        }
    } else if constexpr (CI == TOBIT) {
        *model = F(var_list, optional_par.at(0), optional_par.at(1),
                   GD->dset, opt, GP.prn_ptr());
    }
    save_model(opt);

    return RES;
}
// explicit instantiations for all required (F, CI) pairs
template class GretlModel_NSE_Driver<lsq, OLS>;
template class GretlModel_NSE_Driver<lsq, WLS>;
template class GretlModel_NSE_Driver<logit_probit, LOGIT>;
template class GretlModel_NSE_Driver<logit_probit, PROBIT>;
template class GretlModel_NSE_Driver<count_model, POISSON>;
template class GretlModel_NSE_Driver<count_model, NEGBIN>;
template class GretlModel_NSE_Driver<biprobit_model, BIPROBIT>;
template class GretlModel_NSE_Driver<duration_model, DURATION>;
template class GretlModel_NSE_Driver<heckit_model, HECKIT>;
template class GretlModel_NSE_Driver<logistic_driver, LOGISTIC>;
template class GretlModel_NSE_Driver<ar_model, AR>;
template class GretlModel_NSE_Driver<ar1_model, AR1>;
template class GretlModel_NSE_Driver<lad_model, LAD>;
template class GretlModel_NSE_Driver<garch_to_call, GARCH>;
template class GretlModel_NSE_Driver<mp_ols, MPOLS>;
template class GretlModel_NSE_Driver<panel_model, PANEL>;
template class GretlModel_NSE_Driver<ivreg, IVREG>;
template class GretlModel_NSE_Driver<tobit_model, TOBIT>;



//
// NSE via arima plugin
//
GretlModel_NSE_arima::GretlModel_NSE_arima (GretlDataset *GD,
                                            const std::string &y,
                                            const py::tuple &tup)
                                            : GretlModel_NSE(GD), y(y), tup(tup)  // explicitly call base class constructor!
{
#if GRETL4PY_MODEL_DEBUG
    std::cout << "GretlModel_NSE_arima, id = " << this->get_id();
    std::cout << ", GD  = " << GD << std::endl;
#endif // GRETL4PY_MODEL_DEBUG
}

void GretlModel_NSE_arima::get_arima_order (const py::tuple &tup,
                                            int &p, int &d, int &q)
{
    int n = py::len(tup);

    if (n == 2) {
        p = tup[0].cast<int>();
        q = tup[1].cast<int>();
    } else if (n == 3) {
        p = tup[0].cast<int>();
        d = tup[1].cast<int>();
        q = tup[2].cast<int>();
    } else {
        throw std::runtime_error("order must be of length 2 or 3");
    }
}

py::dict *GretlModel_NSE_arima::fit (const std::string &formula,
                                     const py::kwargs &kwargs)
{
    int seasonal = 0;
    int y_number;
    int i;

    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs(kwargs, ARMA);
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
    get_arima_order(tup, p, d, q);

    if (kwargs.contains("seasonal") && !kwargs["seasonal"].is_none()) {
        if (is_pytuple(kwargs["seasonal"])) {
            const py::tuple seas = kwargs["seasonal"].cast<py::tuple>();

            get_arima_order(seas, P, D, Q);
            if (P > 0 || D > 0 || Q > 0) {
                seasonal = 1;
            }
        }
    }

    model = gretl_model_new();
    var_list = gretl_list_new(seasonal ? 9 : 5);
    if (model == NULL || var_list == NULL || GP.prn_ptr() == NULL) {
        throw std::runtime_error("Failed to allocate memory");
    }

    i = 1;
    var_list[i++] = p;
    var_list[i++] = d;
    var_list[i++] = q;
    if (seasonal) {
        var_list[i++] = LISTSEP;
        var_list[i++] = P;
        var_list[i++] = D;
        var_list[i++] = Q;
    }
    var_list[i++] = LISTSEP;
    var_list[i++] = y_number;

    *model = arma(var_list, NULL, GD->dset, opt, GP.prn_ptr());
    save_model(opt);

    return RES;
}


//
// NSE_Driver_quantreg via quantreg_driver
// extends: GretlModel_NSE_Driver
//
template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver_quantreg<F, CI>::GretlModel_NSE_Driver_quantreg
                                        (GretlDataset *GD, const double &tau)
                                        : GretlModel_NSE_Driver<F, CI>(GD)  // explicitly call base class constructor!
{
    this->optional_par.push_back(tau);
}

template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver_quantreg<F, CI>::GretlModel_NSE_Driver_quantreg
                                        (GretlDataset *GD, const py::list &tau)
                                        : GretlModel_NSE_Driver<F, CI>(GD)  // explicitly call base class constructor!
{
    try {
        this->optional_par = tau.cast<std::vector<double>>();
    } catch (const py::cast_error &e) {
        throw std::runtime_error("invalid 'tau' list: all elements must be numeric");
    }

    this->optional_par_as_mat = gretl_matrix_alloc(this->optional_par.size(), 1);
    for (size_t i=0; i<this->optional_par.size(); i++) {
        gretl_matrix_set(this->optional_par_as_mat, i, 0, this->optional_par.at(i));
    }
}

template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver_quantreg<F, CI>::~GretlModel_NSE_Driver_quantreg (void)
{
    if (this->optional_par_as_mat != NULL) {
        gretl_matrix_free(this->optional_par_as_mat);
    }
}
// explicit instantiations for all required (F, CI) pairs
template class GretlModel_NSE_Driver_quantreg<quantreg_driver, QUANTREG>;
