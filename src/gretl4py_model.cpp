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
extern "C" {
    #include <gretl/var.h>
    #include <gretl/gretl_restrict.h>
    #include <gretl/varprint.h>
}
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
GretlModel::GretlModel (GretlObjType model_obj_type, std::shared_ptr<GretlDataset> GD,
                        const py::kwargs &kwargs)
                        : id(next_model_id++), model_obj_type(model_obj_type),
                        kwargs(kwargs)
{
    if (GD) {
        this->GD = GD;
    }
}

GretlModel::~GretlModel (void)
{
    if (this->GD) {
        this->GD->linked_models_del(this->id);
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

std::unique_ptr<GretlTypes> GretlModel::get_accessor (const std::string &accessor) const
{
    auto res = gretl_generate_gretl_object(accessor.c_str(), err, model_obj_type,
                                           GD->dset, model_ptr());
    if (err) {
        handle_gretl_error(err);
    }

    return res;
}

template<typename T>
void GretlModel::save_model (T *model)
{
    //
    // Here we construct a python dictionary to represent a gretl
    // MODEL, using as intermediate step the gretl_bundle produced by
    // libgretl's bundle_from_model(). We also add the @summary text
    // produced by libgretl's printmodel().
    // FIXME: extend description for VAR, VECM, etc...
    //
    if constexpr (std::is_same_v<T, MODEL>) {
        err = model->errcode;
    } else if constexpr (std::is_same_v<T, GRETL_VAR>) {
        err = model->err;
    }

    if (err) {
        handle_gretl_error(err);
    } else {
        if constexpr (std::is_same_v<T, MODEL>) {
            err = printmodel(model, GD->dset, opt, GP.prn_ptr());
        } else if constexpr (std::is_same_v<T, GRETL_VAR>) {
            err = gretl_VAR_print(model, GD->dset, opt, GP.prn_ptr());
        }
        if (err) {
            handle_gretl_error(err);
        }
        GretlBundle b = GretlBundle(model, GD->dset);
        err = b.err;

        if (err) {
            handle_gretl_error(err);
        } else {
            RES = b.to_dict();
            if (RES.empty()) {
                err = E_DATA;
                throw std::runtime_error("Model bundle to dict conversion failed.");
            } else {
                py::dict _ids;
                //_ids[py::cast("_id")] = model->ID; //FIXME: VARs!!!
                _ids[py::cast("_model_id")] = id;
                RES[py::cast("_ids")] = _ids;
                RES[py::cast("summary")] = GP.get_buffer();
            }

            // we register result
            py::object self = py::cast(this);
            for (auto item : RES) {
                py::setattr(self, item.first, item.second);
            }
            // we link model with dataset used
            GD->linked_models_add(id);
        }
    }
}

void GretlModel::unpack_kwargs (GretlCmdIndex ci)
{
    opt = OPT_NONE;
    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::invalid_argument("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "data") {
            // special case: we use already allocated object of GretlDataset class
            GD = item.second.cast<std::shared_ptr<GretlDataset>>();
            continue;
        } else if (key == "quiet") {
            throw std::invalid_argument("invalid option flag: " + key);
        } else if (key == "seasonal" && ci == ARMA) {
            // arima special: we check seasonal
            if (!is_pytuple(kwargs["seasonal"])) {
                throw std::invalid_argument("seasonal must be a tuple");
            }
            continue;
        } else if (key == "lagselect" && ci == ARMA) {
            throw std::invalid_argument("'lagselect' option for (S)ARIMA is temporarily unavailable");
        } else if ((key == "llimit" || key == "rlimit") && ci == TOBIT) {
            // TOBIT special: we check (l/r)limit
            if (!is_pynumeric(item.second)) {
                throw std::invalid_argument("both llimit and rlimit must be numeric");
            }
            continue;
        } else if (key == "xlist" && (ci == ARMA || ci == VAR || ci == VECM)) {
            // var special: here we only check if xlist is a string, but we must do something with it elsewhere
            if (!is_pystr(item.second)) {
                throw std::invalid_argument("xlist must be a string");
            }
            continue;
        } else if (key == "rxlist" && (ci == VECM)) {
            // var special: here we only check if rxlist is a string, but we must do something with it elsewhere
            if (!is_pystr(item.second)) {
                throw std::invalid_argument("rxlist must be a string");
            }
            continue;
        } else {
            gretl_valid_long_opt(opt, ci, key, item.second);
        }
    }
}

template<typename T>
void GretlModel::do_restrict (T *model, const py::list &impose,
                              const py::kwargs &kwargs)
{
    //
    // we test a restriction given as impose
    //
    py::print("nowy restrict");
    py::dict res;
    gretl_restriction *my_rset = NULL;
    int save_t1 = GD->dset->t1;
    int save_t2 = GD->dset->t2;
    int order = -1;

    // we set up options
    unpack_kwargs_test(opt, "restrict", kwargs, order);

    // this code was rendered from library.c
    for (auto &item : impose) {
        if (!is_pystr(item)) {
            throw std::invalid_argument("given restriction is not string literal");
        }
        if (my_rset == NULL) {
            if constexpr (std::is_same_v<T, MODEL>) {
                my_rset = eqn_restriction_set_start(MapPyToCpp(item).to_string().c_str(),
                                                    model, GD->dset, opt);
            }
            if (my_rset == NULL) {
                err = 1;
                handle_gretl_error(err);
            }
        } else {
            err = restriction_set_parse_line(my_rset,
                                             MapPyToCpp(item).to_string().c_str(),
                                             GD->dset);
            if (err) {
                handle_gretl_error(err);
            }
        }
    }

    // actual work
    if constexpr (std::is_same_v<T, MODEL>) {
        GD->dset->t1 = model->t1;
        GD->dset->t2 = model->t2;
        err = gretl_restriction_finalize(my_rset, GD->dset,
                                         OPT_NONE, GP.prn_ptr());
    }
    GD->dset->t1 = save_t1;
    GD->dset->t2 = save_t2;
    if (err) {
        handle_gretl_error(err);
    }

    // we manufacture resulting object
    res[py::cast("test")] = *get_accessor("$test");
    res[py::cast("pvalue")] = *get_accessor("$pvalue");
    res[py::cast("summary")] = GP.get_buffer();

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "restrictions");

    if (!kwargs.contains("silent")) {
        GP.print();
    }
}

static int unpack_forecast_kwargs (const py::kwargs &kwargs, int *pconf)
{
    int opt = OPT_NONE;
    int conf = 95;
    int err = 0;

    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::invalid_argument("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "quiet") {
            if (!is_pybool(item.second)) {
                throw std::invalid_argument("'quiet' value must be a boolean");
            } else if (item.second.cast<int>() != 0) {
                opt |= OPT_Q;
            }
        } else if (key == "plot") {
            if (!is_pybool(item.second)) {
                // FIXME: in principle this could be a filename
                throw std::invalid_argument("'plot' value must be a boolean");
            } else if (item.second.cast<int>() != 0) {
                opt |= OPT_P;
            }
        } else if (key == "interval") {
            // we want a percentage value for forecast confidence intervals
            if (!is_pyint(item.second)) {
                err = 1;
            } else {
                conf = item.second.cast<int>();
                if (conf < 50 || conf > 99) {
                    err = 1;
                } else {
                    *pconf = conf;
                }
            }
            if (err) {
                throw std::invalid_argument("'interval' value must an integer >= 50 and <= 99");
            }
        }
    }

    return opt;
}

void GretlModel::fr_to_dict (FITRESID *fr, py::dict &fcdict)
{
    gretl_matrix *stats = NULL;
    py::list labels;
    double *tmp = NULL;
    double tval = 0;
    int offset;
    int n_ok;
    int rows;
    int cols;
    int t;

    rows = fr->t2 - fr->t1 + 1;
    offset = fr->nobs - rows;

    fcdict[py::cast("t1")] = fr->t1 + 1;
    fcdict[py::cast("t2")] = fr->t2 + 1;

    cols = 1; // predicted values

    // get count of non-missing actual values
    n_ok = 0;
    for (t=fr->t1; t<=fr->t2; t++) {
        if (!na(fr->actual[t])) {
            n_ok++;
        }
    }
    if (n_ok > 0) {
        cols++; // use actual
        labels.append("actual");
    }
    labels.append("predicted");
    if (fr->sderr != NULL) {
        double a2 = fr->alpha / 2.0;

        cols += 3; // add sderr + add confidence interval
        labels.append("stderr");
        labels.append("low");
        labels.append("high");
        tval = (fr->asymp) ? normal_critval(a2) : student_critval(fr->df, a2);
    }

    // build array holding up to five columns
    DNumPyArrayFC F({rows, cols});
    tmp = F.mutable_data();
    if (n_ok > 0) {
        std::memcpy(tmp, fr->actual + offset, rows * sizeof *tmp);
        tmp += rows;
    }
    std::memcpy(tmp, fr->fitted + offset, rows * sizeof *tmp);
    if (fr->sderr != NULL) {
        double maxerr;
        int i;

        tmp += rows;
        std::memcpy(tmp, fr->sderr + offset, rows * sizeof *tmp);
        tmp += rows;
        for (i=0; i<rows; i++) {
            maxerr = tval * fr->sderr[offset + i];
            tmp[i] = fr->fitted[offset + i] - maxerr;
            tmp[i+rows] = fr->fitted[offset + i] + maxerr;
        }
        fcdict[py::cast("confidence")] = 100 * (1 - fr->alpha);
    }

    fcdict[py::cast("data")] = F;
    fcdict[py::cast("data_labels")] = labels;

    n_ok = 0;
    stats = forecast_stats(fr->actual, fr->fitted, fr->t1, fr->t2,
                           &n_ok, OPT_O, &err);
    if (stats != NULL) {
        if (n_ok > 0) {
            const char **rnames = gretl_matrix_get_rownames(stats);
            const char *label;
            // it seems we need a nominally 2-d array here for tabulate()
            DNumPyArrayFC S({stats->rows, 1});
            py::list stats_labels;

            tmp = S.mutable_data();
            std::memcpy(tmp, stats->val, stats->rows * sizeof *tmp);
            fcdict[py::cast("stats")] = S;
            for (t=0; t<stats->rows; t++) {
                label = rnames[t];
                stats_labels.append(label);
            }
            fcdict[py::cast("stats_labels")] = stats_labels;
        }
        gretl_matrix_free(stats);
    }
}


//////////////////////////////////////////////////
// single equation models
//////////////////////////////////////////////////
//
// Native Single Equation libgretl MODEL (NSE)
// extends: GretlModel
//
GretlModel_NSE::GretlModel_NSE (std::shared_ptr<GretlDataset> GD, const py::kwargs &kwargs)
                                : GretlModel(GRETL_OBJ_EQN, GD, kwargs)
{}

GretlModel_NSE::~GretlModel_NSE (void)
{
    if (this->model != NULL) {
        gretl_model_free(this->model);
    }
}

void *GretlModel_NSE::model_ptr (void) const
{
    return static_cast<void*>(model);
}

py::dict GretlModel_NSE::forecast (const py::kwargs &kwargs)
{
    py::dict res;
    FITRESID *fr = NULL;
    FcastFlags flags = static_cast<FcastFlags>(0);
    gretlopt opt = OPT_NONE;
    int t1, t2 = 0;
#if 0 // not yet
    int recursive = 0, k = 1, *kptr;
#endif
    int dt2 = GD->dset->n - 1;
    int st2 = GD->dset->n - 1;
    int conf = 95;

    // check for possible sample mismatch
    err = model_sample_problem(model, GD->dset);
    if (err) {
        handle_gretl_error(err);
    }

    // try to figure which fcast options might be applicable
    forecast_options_for_model(model, GD->dset, &flags, &dt2, &st2);

    if (flags & (FC_DYNAMIC_OK | FC_AUTO_OK)) {
        t2 = dt2;
    } else {
        t2 = st2;
    }

    // for now, we just do a default out-of-sample forecast
    if (t2 <= model->t2) {
        throw std::runtime_error("there are no suitable observations for prediction");
    } else {
        t1 = model->t2 + 1;
    }

    // assemble options
    opt = (gretlopt) unpack_forecast_kwargs(kwargs, &conf);
    fr = get_forecast(model, t1, t2, 0, GD->dset, OPT_NONE, &err);

    if (err) {
        handle_gretl_error(err);
    } else {
        fr->alpha = (100.0 - conf) / 100.0;
        fr_to_dict(fr, res);
        text_print_forecast(fr, GD->dset, opt, GP.prn_ptr());
    }

    if (fr != NULL) {
        free_fit_resid(fr);
    }

    if (!(opt & OPT_Q)) {
        GP.print();
    }

    return res;
}

void GretlModel_NSE::test (const std::string &type,
                           const py::kwargs &kwargs)
{
    //
    // this method performs test which do not take a variables list
    //
    py::dict res;
    int order = GD->dset->pd;

    // we set up options
    unpack_kwargs_test(opt, type, kwargs, order);

    // we perform the test
    if (type == "bkw") {
        err = bkw_test(model, GD->dset, opt, GP.prn_ptr());
    } else if (type == "cusum") {
        err = cusum_test(model, GD->dset, opt, GP.prn_ptr());
    } else if (type == "leverage") {
        err = leverage_test(model, GD->dset, opt, GP.prn_ptr());
    } else if (type == "panspec") {
        err = panel_specification_test(model, GD->dset, opt, GP.prn_ptr());
    } else if (type == "reset") {
        err = reset_test(model, GD->dset, opt, GP.prn_ptr());
    } else if (type == "qlrtest") {
        err = QLR_test(model, GD->dset, opt, GP.prn_ptr());
    } else if (type == "vif") {
        err = vif_test(model, GD->dset, opt, GP.prn_ptr());
    } else if (type == "arch" || type == "autocorr" ||
               type == "breusch-pagan" || type == "comfac" ||
               type == "logs" || type == "normality" ||
               type == "panel" || type == "squares" ||
               type == "white" || type == "white-nocross" ||
               type == "xdepend" ) {
        if (get_last_model(NULL) != model) {
            err = gretl_model_protect(model);
            if (err) {
                handle_gretl_error(err);
            }
            set_as_last_model(model, model_obj_type);
        }
        err = model_test_driver(order, GD->dset, opt, GP.prn_ptr());
    } else {
        throw std::invalid_argument(type + " is a dataset-based test");
    }
    if (err) {
        handle_gretl_error(err);
    }

    // we manufacture resulting object
    res[py::cast("type")] = type;
    res[py::cast("options")] = kwargs;
    res[py::cast("test")] = *get_accessor("$test");
    res[py::cast("pvalue")] = *get_accessor("$pvalue");
    res[py::cast("summary")] = GP.get_buffer();

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "tests");

    if (!kwargs.contains("silent")) {
        GP.print();
    }
}

void GretlModel_NSE::test (const std::string &type, const std::string &vars,
                           const py::kwargs &kwargs)
{
    //
    // this method performs test which do take a variables list
    //
    py::dict res;
    int order = -1;
    gretl_list_ptr ex_var{nullptr};

    // we construct list of external variables
    if (type != "chow") {
        ex_var = list_from_formula(vars, GD->dset, err); // it's std::unique_ptr!!!
        if (!ex_var) {
            throw std::invalid_argument("wrong list of variables");
        }
    }

    // we set up options
    unpack_kwargs_test(opt, type, kwargs, order);

    // we perform the test
    if (type == "add") {
        err = add_test(model, ex_var.get(), GD->dset, opt, GP.prn_ptr());
    } else if (type == "chow") {
        err = chow_test_driver(vars.c_str(), model, GD->dset, opt, GP.prn_ptr());
    } else if (type == "coeffsum") {
        err = gretl_sum_test(ex_var.get(), model, GD->dset, opt, GP.prn_ptr());
    } else if (type == "omit") {
        err = omit_test(model, ex_var.get(), GD->dset, opt, GP.prn_ptr());
    } else {
        throw std::invalid_argument(type + " is a dataset-based test");
    }
    if (err) {
        handle_gretl_error(err);
    }

    // we manufacture resulting object
    res[py::cast("type")] = type;
    res[py::cast("options")] = kwargs;
    res[py::cast("test")] = *get_accessor("$test");
    res[py::cast("pvalue")] = *get_accessor("$pvalue");
    res[py::cast("summary")] = GP.get_buffer();

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "tests");

    if (!kwargs.contains("silent")) {
        GP.print();
    }
}

void GretlModel_NSE::restrict (const py::list &impose,
                               const py::kwargs &kwargs)
{
    do_restrict<MODEL>(model, impose, kwargs);
}

/*
void GretlModel_NSE::restrict (const py::list &impose,
                               const py::kwargs &kwargs)
{
    //
    // we test a restriction given as impose
    //
    py::dict res;
    gretl_restriction *my_rset = NULL;
    int save_t1 = GD->dset->t1;
    int save_t2 = GD->dset->t2;
    int order = -1;

    // we set up options
    unpack_kwargs_test(opt, "restrict", kwargs, order);

    // this code was rendered from library.c
    for (auto &item : impose) {
        if (!is_pystr(item)) {
            throw std::invalid_argument("given restriction is not string literal");
        }
        if (my_rset == NULL) {
            my_rset = eqn_restriction_set_start(MapPyToCpp(item).to_string().c_str(),
                                                model, GD->dset, opt);
            if (my_rset == NULL) {
                err = 1;
                handle_gretl_error(err);
            }
        } else {
            err = restriction_set_parse_line(my_rset,
                                             MapPyToCpp(item).to_string().c_str(),
                                             GD->dset);
            if (err) {
                handle_gretl_error(err);
            }
        }
    }

    // actual work
    GD->dset->t1 = model->t1;
    GD->dset->t2 = model->t2;
    err = gretl_restriction_finalize(my_rset, GD->dset, OPT_NONE, GP.prn_ptr());
    GD->dset->t1 = save_t1;
    GD->dset->t2 = save_t2;
    if (err) {
        handle_gretl_error(err);
    }

    // we manufacture resulting object
    res[py::cast("test")] = *get_accessor("$test");
    res[py::cast("pvalue")] = *get_accessor("$pvalue");
    res[py::cast("summary")] = GP.get_buffer();

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "restrictions");

    if (!kwargs.contains("silent")) {
        GP.print();
    }
}
*/


//
// NSE via dedicated Driver
//
template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver<F, CI>::GretlModel_NSE_Driver (std::shared_ptr<GretlDataset> GD,
                                                     const std::string &formula,
                                                     const py::kwargs &kwargs)
                                                     : GretlModel_NSE(GD, kwargs), formula(formula)
{
    if constexpr (CI == TOBIT) {
        optional_par.resize(2);
        optional_par.at(0) = NADBL;
        optional_par.at(1) = NADBL;
    }
}

template <auto F, GretlCmdIndex CI>
std::string GretlModel_NSE_Driver<F, CI>::get_formula (void) const
{
    return formula;
}

// Method template to support several estimation methods that
// have the same "basic" signature, plus a few special cases
// F: function to call
// CI: GretlCmdIndex
template <auto F, GretlCmdIndex CI>
void GretlModel_NSE_Driver<F, CI>::fit (void)
{
    if (py::len(kwargs) > 0) {
        unpack_kwargs(CI);
    }

    if (!GD) {
        throw std::runtime_error("No dataset is available");
    }

    // TOBIT special: we set (l/r)limit
    if constexpr (CI == TOBIT) {
        if (kwargs.contains("llimit") && !kwargs["llimit"].is_none()) {
            optional_par.at(0) = py::cast<double>(kwargs["llimit"]);
        }
        if (kwargs.contains("rlimit") && !kwargs["rlimit"].is_none()) {
            optional_par.at(1) = py::cast<double>(kwargs["rlimit"]);
        }
    }

    // we construct the model
    var_list = list_from_formula(formula, GD->dset, err); // it's std::unique_ptr!!!
    model = gretl_model_new();

    if (!var_list) {
        throw std::runtime_error("Formula rendering failed");
    } else if (model == NULL || GP.prn_ptr() == NULL) {
        throw std::runtime_error("Failed to allocate memory");
    }

    // we call F as an estimator
    if constexpr (use_ldco_sig(CI)) {
        *model = F(var_list.get(), GD->dset, CI, opt);
    } else if constexpr (use_ldcop_sig(CI)) {
        *model = F(var_list.get(), GD->dset, CI, opt, GP.prn_ptr());
    } else if constexpr (use_ldo_sig(CI)) {
        *model = F(var_list.get(), GD->dset, opt);
    } else if constexpr (use_ldop_sig(CI)) {
        *model = F(var_list.get(), GD->dset, opt, GP.prn_ptr());
    } else if constexpr (use_lcdop_sig(CI)) {
        *model = F(var_list.get(), CI, GD->dset, opt, GP.prn_ptr());
    } else if constexpr (CI == QUANTREG) {
        if (this->optional_par_as_mat != NULL) {
            *model = quantreg(this->optional_par_as_mat,
                              var_list.get(), GD->dset, opt, GP.prn_ptr());
        } else {
            *model = F(std::to_string(this->optional_par.at(0)).c_str(),
                       var_list.get(), GD->dset, opt, GP.prn_ptr());
        }
    } else if constexpr (CI == TOBIT) {
        *model = F(var_list.get(), optional_par.at(0), optional_par.at(1),
                   GD->dset, opt, GP.prn_ptr());
    }
    save_model<MODEL>(model);
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
template class GretlModel_NSE_Driver<garch, GARCH>;
template class GretlModel_NSE_Driver<mp_ols, MPOLS>;
template class GretlModel_NSE_Driver<panel_model, PANEL>;
template class GretlModel_NSE_Driver<ivreg, IVREG>;
template class GretlModel_NSE_Driver<tobit_model, TOBIT>;


//
// NSE via arima plugin
//
GretlModel_NSE_arima::~GretlModel_NSE_arima (void) = default;

GretlModel_NSE_arima::GretlModel_NSE_arima (std::shared_ptr<GretlDataset> GD,
                                            const std::string &y, const py::tuple &tup,
                                            const py::kwargs &kwargs)
                                            : GretlModel_NSE(GD, kwargs), y(y), tup(tup)
{}

std::string GretlModel_NSE_arima::get_formula (void) const
{
    return y;
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
        throw std::invalid_argument("order must be of length 2 or 3");
    }
}

void GretlModel_NSE_arima::fit (void)
{
    int seasonal = 0;
    int exogenous = 0;
    gretl_list_ptr xlist;
    int y_number;
    int i;

    if (py::len(kwargs) > 0) {
        unpack_kwargs(ARMA);
    }

    if (!GD) {
        throw std::runtime_error("No dataset is available");
    }

    // we set up "y"
    y_number = current_series_index(GD->dset, y.c_str());
    if (y_number == -1) {
        throw std::invalid_argument("There is no variable: " + y);
    }

    // we set up non-seasonal parameters
    p = d = q = 0;
    get_arima_order(tup, p, d, q);

    // we set up seasonal parameters
    if (kwargs.contains("seasonal") && !kwargs["seasonal"].is_none()) {
        const py::tuple seas = kwargs["seasonal"].cast<py::tuple>();
        P = D = Q = 0;

        get_arima_order(seas, P, D, Q);
        if (P > 0 || D > 0 || Q > 0) {
            seasonal = 1;
        }
    }

    // we add exogenous variables if present
    if (kwargs.contains("xlist") && !kwargs["xlist"].is_none()) {
        xlist = list_from_formula(kwargs["xlist"].cast<std::string>(), GD->dset, err);
        if (!xlist) {
            throw std::invalid_argument("wrong list of exogenous variables");
        } else if (err) {
            handle_gretl_error(err);
        }
        exogenous = xlist.get()[0];
    }

    model = gretl_model_new();
    var_list.reset(gretl_list_new(seasonal ? 9+exogenous : 5+exogenous)); // it's std::unique_ptr!!!
    if (model == NULL || var_list == nullptr || GP.prn_ptr() == NULL) {
        throw std::runtime_error("Failed to allocate memory");
    }

    i = 1;
    var_list.get()[i++] = p;
    var_list.get()[i++] = d;
    var_list.get()[i++] = q;
    if (seasonal) {
        var_list.get()[i++] = LISTSEP;
        var_list.get()[i++] = P;
        var_list.get()[i++] = D;
        var_list.get()[i++] = Q;
    }
    var_list.get()[i++] = LISTSEP;
    var_list.get()[i++] = y_number;
    if (exogenous) {
        for (int j=1; j<=exogenous; j++) {
            var_list.get()[i++] = xlist.get()[j];
        }
    }

    *model = arma(var_list.get(), NULL, GD->dset, opt, GP.prn_ptr());
    save_model<MODEL>(model);
}


//
// NSE_Driver_quantreg via quantreg_driver
// extends: GretlModel_NSE_Driver
//
template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver_quantreg<F, CI>::GretlModel_NSE_Driver_quantreg
                                        (std::shared_ptr<GretlDataset> GD,
                                         const std::string &formula,
                                         const double &tau,
                                         const py::kwargs &kwargs)
                                        : GretlModel_NSE_Driver<F, CI>(GD, formula, kwargs)
{
    this->optional_par.push_back(tau);
}

template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver_quantreg<F, CI>::GretlModel_NSE_Driver_quantreg
                                        (std::shared_ptr<GretlDataset> GD,
                                         const std::string &formula,
                                         const py::array_t<double> &tau,
                                         const py::kwargs &kwargs)
                                        : GretlModel_NSE_Driver<F, CI>(GD, formula, kwargs)
{
    if (tau.ndim() != 1) {
        throw std::invalid_argument("'tau' is expected to be a vector");
    }
    this->optional_par_as_mat = gretl_matrix_alloc(tau.shape(0), 1);
    for (int i=0; i<tau.shape(0); i++) {
        gretl_matrix_set(this->optional_par_as_mat, i, 0, tau.at(i));
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


//
// VAR libgretl MODEL
// extends: GretlModel
//
GretlModel_VAR::GretlModel_VAR (std::shared_ptr<GretlDataset> GD, const std::string &ylist,
                                const int order, const py::kwargs &kwargs)
                                : GretlModel(GRETL_OBJ_VAR, GD, kwargs),
                                vars(ylist), order(order)
{}

GretlModel_VAR::~GretlModel_VAR (void)
{
    if (this->model != NULL) {
        gretl_VAR_free(this->model);
    }
}

void *GretlModel_VAR::model_ptr (void) const
{
    return static_cast<void*>(model);
}

// true VAR/VECM estimate
template <GretlCmdIndex CI>
void GretlModel_VAR::do_var_vecm_fit (const int coint_rank)
{
    //
    // here we perform a real VAR/VECM estimation
    // Note: for VAR case we do not use rank, of course
    //
    if (py::len(kwargs) > 0) {
        unpack_kwargs(CI);
    }

    if (!GD) {
        throw std::runtime_error("No dataset is available");
    }

    // we add exogenous variables (if present)
    if (kwargs.contains("xlist") && !kwargs["xlist"].is_none()) {
        vars.append(" ; " + kwargs["xlist"].cast<std::string>());
    }
    // we add exogenous variables restricted to the cointegration space (if present)
    if constexpr (CI == VECM) {
        if (kwargs.contains("rxlist") && !kwargs["rxlist"].is_none()) {
            vars.append(" ; " + kwargs["rxlist"].cast<std::string>());
        }
    }

    // be build variables list
    var_list = list_from_formula(vars, GD->dset, err);
    if (!var_list) {
        throw std::invalid_argument("wrong list of variables");
    }

    // we estimate a model
    if constexpr (CI == VAR) {
        model = gretl_VAR(order, NULL, var_list.get(), GD->dset,
                          opt, GP.prn_ptr(), &err);
    } else if constexpr (CI == VECM) {
        model = gretl_VECM(order, coint_rank, var_list.get(), GD->dset,
                           opt, GP.prn_ptr(), &err);
    }

    if (err) {
        handle_gretl_error(err);
    }
    save_model<GRETL_VAR>(model);
}

void GretlModel_VAR::fit (void)
{
    do_var_vecm_fit<VAR>();
}

py::dict GretlModel_VAR::forecast (const py::kwargs &kwargs)
{
    py::print("GretlModel_VAR::forecast(): not implemented yet!");

    return py::dict();
}

void GretlModel_VAR::restrict (const py::list &impose, const py::kwargs &kwargs)
{
    py::print("GretlModel_VAR::restrict(): not implemented yet!");
}

void GretlModel_VAR::test (const std::string &type, const py::kwargs &kwargs)
{
    py::dict res;
    int order = GD->dset->pd;

    // basic check
    if (type == "normality" && kwargs.contains("order")) {
        throw std::invalid_argument("normality test doesn't take 'order' option");
    }

    // we set up options
    unpack_kwargs_test(opt, type, kwargs, order);

    // we perform test
    if (type == "autocorr") {
        err = gretl_VAR_autocorrelation_test(model, order, GD->dset, opt, GP.prn_ptr());
    } else if (type == "arch") {
        err = gretl_VAR_arch_test(model, order, GD->dset, opt, GP.prn_ptr());
    } else if (type == "normality") {
        err = gretl_VAR_normality_test(model, opt, GP.prn_ptr());
    } else {
        throw std::invalid_argument(type + ": unsupported test");
    }
    if (err) {
        handle_gretl_error(err);
    }

     // we manufacture resulting object
    res[py::cast("type")] = type;
    res[py::cast("options")] = kwargs;
    res[py::cast("test")] = *get_accessor("$test");
    res[py::cast("pvalue")] = *get_accessor("$pvalue");
    res[py::cast("summary")] = GP.get_buffer();

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "tests");

    if (!kwargs.contains("silent")) {
        GP.print();
    }
}

std::string GretlModel_VAR::get_formula() const
{
    return vars;
}

void GretlModel_VAR::irf (int targ, int shock, double alpha)
{
    //
    // this method mimics gretl's irf() function
    //
    GretlType t = GRETL_TYPE_MATRIX;
    gretl_matrix *irf = NULL;
    py::dict res;

    // we compute IRFs
    irf = gretl_VAR_get_impulse_response(model, targ, shock,
                                         default_VAR_horizon(GD->dset),
                                         alpha, GD->dset, &err);
    if (err) {
        handle_gretl_error(err);
    }

    // we manufacture resulting object
    res[py::cast("targ")] = targ;
    res[py::cast("targ_name")] = GD->dset->varname[gretl_VAR_get_variable_number(model, targ)];
    res[py::cast("shock")] = shock;
    res[py::cast("shock_name")] = GD->dset->varname[gretl_VAR_get_variable_number(model, shock)];
    if (alpha > 0.0) {
        res[py::cast("alpha")] = alpha;
    }
    res[py::cast("results")] = std::move(*convert_GretlType_to_GretlTypes(t, irf, 0, err));

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "irfs");

    // release a memory
    if (irf != NULL) {
        gretl_matrix_free(irf);
    }
}

void GretlModel_VAR::irf (const std::string &targ, const std::string &shock, double alpha)
{
    //
    // this method mimics gretl's irf() function
    // Note: overload which takes names instead of equation number
    //
    int targ_int;
    int shock_int;

    // here we looking for targ and shock IDs
    auto find_idx = [](const std::string &names, const std::string &val) -> int
    {
        std::istringstream iss(names);
        std::string token;
        int pos = 0;

        while (iss >> token) {
            if (token == ";") {
                break;
            }
            if (token == val) {
                return pos;
            }
            ++pos;
        }

        return -1; // not found
    };

    // let's try to find our targ/shock IDs
    try {
        targ_int = std::stoi(targ);
    } catch (std::invalid_argument &e) {
        targ_int = find_idx(vars, targ);
    }
    try {
        shock_int = std::stoi(shock);
    } catch (std::invalid_argument &e) {
        shock_int = find_idx(vars, shock);
    }

    if (targ_int == -1) {
        throw std::invalid_argument("targ value is invalid");
    }
    if (shock_int == -1) {
        throw std::invalid_argument("shock value is invalid");
    }

    irf(targ_int, shock_int, alpha);
}

//
// VECM libgretl MODEL
// extends: GretlModel_VAR
//
GretlModel_VAR_VECM::GretlModel_VAR_VECM (std::shared_ptr<GretlDataset> GD, const std::string &ylist,
                                          const int order, const int rank, const py::kwargs &kwargs)
                                          : GretlModel_VAR(GD, ylist, order, kwargs), coint_rank(rank)
{}

void GretlModel_VAR_VECM::fit (void)
{
    do_var_vecm_fit<VECM>(coint_rank);
}
