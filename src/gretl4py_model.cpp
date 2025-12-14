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

#include "gretl4py_model.h"
#include "gretl_calls.h"
extern "C" {
    #include <gretl/var.h>
    #include <gretl/gretl_restrict.h>
    #include <gretl/varprint.h>
    #include <gretl/system.h>
}
#include <gretl/forecast.h>
#include <string>
#include <regex>
#include <algorithm>

#define use_ldco_sig(c) (c == OLS || c == WLS) //const int *list, DATASET *dset, GretlCmdIndex ci, gretlopt opt
#define use_ldcop_sig(c) (c == LOGIT || c == PROBIT) //int *list, DATASET *dset, int ci, gretlopt opt, PRN *prn
#define use_ldo_sig(c) (c == LOGISTIC || c == LAD \
                        || c == MPOLS || c == IVREG || c == HSK) //const int *list, DATASET *dset, gretlopt opt
#define use_ldop_sig(c) (c == BIPROBIT || c == DURATION \
                         || c == HECKIT || c == AR || c == AR1 \
                         || c == GARCH || c == PANEL || c == INTREG) //const int *list, DATASET *dset, gretlopt opt, PRN *prn
#define use_lcdop_sig(c) (c == POISSON || c == NEGBIN) //const int *list, int ci, DATASET *dset, gretlopt opt, PRN *prn

#define GRETL4PY_MODEL_DEBUG 0

extern GretlDatasetPtr DSET;
extern std::unordered_set<std::string> UserVars;

//
// GretlModel class methods
//
GretlModel::GretlModel (GretlObjType model_obj_type, GretlCmdIndex ci,
                        const py::kwargs &kwargs)
                        : id_(next_model_id_++), model_obj_type_(model_obj_type),
                        ci_(ci), GD_(DSET)
{
    // we unpack kwargs
    if (py::len(kwargs) && ci_ != NC) {
        for (auto &item : kwargs) {
            if (!is_pystr(item.first)) {
                throw std::invalid_argument("option key must be a string");
            }
            const std::string &key = item.first.cast<std::string>();

            if (key == "data") {
                // special case: we use already allocated object of GretlDataset class
                this->GD_ = item.second.cast<GretlDatasetPtr>();
                continue;
            } else if (key == "quiet" && !(ci_ == VAR || ci_ == VECM)) {
                throw std::invalid_argument("invalid option flag: " + key);
            } else if (key == "seasonal" && ci_ == ARMA) {
                // arima special: we check seasonal
                if (!is_pytuple(kwargs["seasonal"])) throw std::invalid_argument("'seasonal' must be a tuple");
                continue;
            } else if (key == "lagselect" && (ci_ == ARMA || ci_ == VAR)) {
                throw std::invalid_argument("'lagselect' option for (S)ARIMA and VAR is temporarily unavailable");
            } else if ((key == "llimit" || key == "rlimit") && ci_ == TOBIT) {
                // TOBIT special: we check (l/r)limit
                if (!is_pynumeric(item.second)) throw std::invalid_argument("both 'llimit' and 'rlimit' must be numeric");
                continue;
            } else if (key == "xlist" && (ci_ == ARMA || ci_ == VAR || ci_ == VECM)) {
                // var special: here we only check if xlist is a string, but we must do something with it elsewhere
                if (!is_pystr(item.second)) throw std::invalid_argument("'xlist' must be a string");
                continue;
            } else if (key == "rxlist" && ci_ == VECM) {
                // var special: here we only check if rxlist is a string, but we must do something with it elsewhere
                if (!is_pystr(item.second)) throw std::invalid_argument("'rxlist' must be a string");
                continue;
            } else if (key == "instruments" && ci_ == DPANEL) {
                // var special: here we only check if instruments is a string, but we must do something with it elsewhere
                if (!is_pystr(item.second)) throw std::invalid_argument("'instruments' must be a string");
            } else if ((key == "params" || key == "param_names" || key == "initvals") && (ci_ == NLS || ci_ == MLE || ci_ == GMM)) {
                // NLS/MLE/GMM special case: we deal with these in class constructor
                continue;
            } else if (key == "deriv" && (ci_ == NLS || ci_ == MLE)) {
                // NLS/MLE/GMM special case: we deal with these in class constructor
                continue;
            } else if ((key == "orthog" || key == "weights") && ci_ == GMM) {
                // GMM special case: we deal with these in class constructor
                continue;
            } else if (key == "statements" && (ci_ == MLE || ci_ == GMM)) {
                // MLE/GMM special case: we deal with these in class constructor
                continue;
            } else if ((key == "equations" || key == "instruments" || key == "endogs" || key == "identities") && ci_ == SYSTEM) {
                // SYSTEM special case: we deal with these in class constructor
                continue;
            } else {
                gretl_valid_long_opt(opt_, ci_, key, item.second);
            }
        }
    }

    // we check if we have dataset
    if (!GD_) handle_gretl_error(E_NODATA);
}

GretlModel::~GretlModel (void)
{
    if (this->GD_) this->GD_->linked_models_del(this->id_);
}

int GretlModel::get_id (void) const
{
    return id_;
}

void GretlModel::unlink_from_dataset (void) const
{
    if (GD_) GD_->linked_models_del(id_);
}

py::object GretlModel::get_accessor (const std::string &accessor) const
{
    auto res = gretl_generate_gretl_object(accessor.c_str(), err_,
                                           model_obj_type_,
                                           GD_->dset, model_ptr_());
    if (err_) handle_gretl_error(err_);

    return variant_to_pyobject(std::move(*res));
}

template <typename T>
void GretlModel::save_model_ (T *model)
{
    //
    // Here we construct a python dictionary to represent a gretl
    // MODEL, using as intermediate step the gretl_bundle produced by
    // libgretl's bundle_from_model(). We also add the @summary text
    // produced by libgretl's printmodel().
    // FIXME: extend description for VAR, VECM, etc...
    //
    if constexpr (std::is_same_v<T, MODEL>) {
        err_ = model->errcode;
    } else if constexpr (std::is_same_v<T, GRETL_VAR>) {
        err_ = model->err;
    }
    if (err_) handle_gretl_error(err_);

    // we print model
    if constexpr (std::is_same_v<T, MODEL>) {
        // we forse Pooled OLS heading if needed
        if (gretl_model_get_int(model, "pooled")) {
            opt_ = static_cast<gretlopt>(opt_ & ~OPT_P);
        }
        err_ = printmodel(model, GD_->dset, opt_,
                          GP_.prn_ptr());
    } else if constexpr (std::is_same_v<T, GRETL_VAR>) {
        err_ = gretl_VAR_print(model, GD_->dset,
                               opt_, GP_.prn_ptr());
    } else if constexpr (std::is_same_v<T, equation_system>) {
        // FIXME: needed?
        err_ = gretl_system_print(model, GD_->dset,
                                  opt_, GP_.prn_ptr());
    } else {
        static_assert(always_false<T>, "GretlModel::save_model_(): unknown type");
    }
    if (err_) handle_gretl_error(err_);
    summary_ = GP_.get_buffer();

    // we produce $model bundle based on estimated @model
    if constexpr (std::is_same_v<T, equation_system>) {
        GB_model_ = std::make_shared<GretlBundle>(model);
    } else {
        GB_model_ = std::make_shared<GretlBundle>(model, GD_->dset);
    }
    err_ = GB_model_->get_err();
    if (err_) handle_gretl_error(err_);

    // we register result on python side
    py::object self = py::cast(this);
    GB_model_->python_register(self);

    py::dict _ids;
    _ids[py::cast("_model_id")] = id_;
    py::setattr(self, "_ids", _ids);

    // we set some internal stuff
    GD_->linked_models_add(id_);
    estimated_ = true;
}

template <typename T>
void GretlModel::do_restrict_ (T *model, const py::list &impose,
                               const py::kwargs &kwargs)
{
    //
    // we test a restriction given as impose
    // Note: this method is used in GretlModel_NSE
    // GretlModel_VAR_VECM, and GretlModel_NME
    //
    py::dict res;
    gretl_restriction *my_rset = NULL;
    int save_t1 = GD_->dset->t1;
    int save_t2 = GD_->dset->t2;
    int order = -1;

    // we set up options
    unpack_kwargs_test(opt_, "restrict", kwargs, order);

    // this code was rendered from library.c
    for (auto &item : impose) {
        if (!is_pystr(item)) {
            throw std::invalid_argument("given restriction is not string literal");
        }
        if (my_rset == NULL) {
            if constexpr (std::is_same_v<T, MODEL>) {
                my_rset = eqn_restriction_set_start(MapPyToCpp(item).to_string().c_str(),
                                                    model, GD_->dset, opt_);
            } else if constexpr (std::is_same_v<T, GRETL_VAR>) {
                my_rset = var_restriction_set_start(MapPyToCpp(item).to_string().c_str(),
                                                    model);
            } else if constexpr (std::is_same_v<T, equation_system>) {
                my_rset = cross_restriction_set_start(MapPyToCpp(item).to_string().c_str(),
                                                      model);
            }
            if (my_rset == NULL) {
                err_ = 1;
                handle_gretl_error(err_);
            }
        } else {
            err_ = restriction_set_parse_line(my_rset,
                                              MapPyToCpp(item).to_string().c_str(),
                                              GD_->dset);
            if (err_) handle_gretl_error(err_);
        }
    }

    // actual work
    if constexpr (std::is_same_v<T, GRETL_VAR>) {
        GRETL_VAR *vnew = NULL;

        vnew = gretl_restricted_vecm(my_rset, GD_->dset, opt_,
                                     GP_.prn_ptr(), &err_);
        if (vnew != NULL) {
            gretl_VAR_free(vnew);
        }
    } else {
        GD_->dset->t1 = model->t1;
        GD_->dset->t2 = model->t2;
        err_ = gretl_restriction_finalize(my_rset, GD_->dset,
                                          OPT_NONE, GP_.prn_ptr());
    }
    if constexpr (std::is_same_v<T, equation_system>) {
        err_ = equation_system_estimate(model, GD_->dset,
                                        OPT_NONE, GP_.prn_ptr());
    }
    GD_->dset->t1 = save_t1;
    GD_->dset->t2 = save_t2;
    if (err_) handle_gretl_error(err_);

    // we manufacture resulting object
    res[py::cast("test")] = get_accessor("$test");
    res[py::cast("pvalue")] = get_accessor("$pvalue");
    res[py::cast("summary")] = GP_.get_buffer();

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "restrictions");

    if (!kwargs.contains("silent")) {
        GP_.print();
    }
}

template <typename T, GretlCmdIndex CI>
void GretlModel::do_forecast_ (T *model, const py::kwargs &kwargs,
                               int variable_nr)
{
    //
    // here we perform native libgretl forecasting for NSE, VAR, VECM
    // Note: variable_nr is used only for systems and indicates variable
    // for which we generate forecasts
    //
    py::dict res;
    FITRESID *fr = NULL;
    FcastFlags flags = static_cast<FcastFlags>(0);
    int t1, t2 = 0;
#if 0 // not yet
    int recursive = 0, k = 1, *kptr;
#endif
    int dt2 = GD_->dset->n - 1;
    int st2 = GD_->dset->n - 1;
    int conf = 95;

    // single equation models only
    if constexpr (std::is_same_v<T, MODEL>) {
        // check for possible sample mismatch
        err_ = model_sample_problem(model, GD_->dset);
        if (err_) handle_gretl_error(err_);

        // try to figure which fcast options might be applicable
        forecast_options_for_model(model, GD_->dset, &flags, &dt2, &st2);
    }

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
    unpack_forecast_kwargs_(kwargs, conf);

    // actual work
    if constexpr (std::is_same_v<T, MODEL>) {
        fr = get_forecast(model, t1, t2, 0, GD_->dset, OPT_NONE, &err_);
    } else if constexpr (std::is_same_v<T, GRETL_VAR>) {
        fr = get_system_forecast(model, CI, variable_nr, t1, t2,
                                 0, GD_->dset, OPT_NONE, &err_);
    } else {
        static_assert(always_false<T>, "GretlModel::do_forecast_(): unknown type");
    }
    if (err_) handle_gretl_error(err_);
    fr->alpha = (100.0 - conf) / 100.0;
    fr_to_dict_(fr, res);
    text_print_forecast(fr, GD_->dset, opt_, GP_.prn_ptr());
    if (fr != NULL) {
        free_fit_resid(fr);
    }
    if (!(opt_ & OPT_Q)) {
        GP_.print();
    }

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "forecasts");
}

void GretlModel::unpack_forecast_kwargs_ (const py::kwargs &kwargs, int &pconf)
{
    int conf = 95;

    opt_ = OPT_NONE;
    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) throw std::invalid_argument("option key must be a string");
        const std::string &key = item.first.cast<std::string>();

        if (key == "quiet") {
            if (!is_pybool(item.second)) {
                throw std::invalid_argument("'quiet' value must be a boolean");
            } else if (item.second.cast<int>() != 0) {
                SET_OPT(opt_, OPT_Q);
            }
        } else if (key == "plot") {
            if (!is_pybool(item.second)) {
                // FIXME: in principle this could be a filename
                throw std::invalid_argument("'plot' value must be a boolean");
            } else if (item.second.cast<int>() != 0) {
                SET_OPT(opt_, OPT_P);
            }
        } else if (key == "interval") {
            // we want a percentage value for forecast confidence intervals
            if (!is_pyint(item.second)) {
                err_ = 1;
            } else {
                conf = item.second.cast<int>();
                if (conf < 50 || conf > 99) {
                    err_ = 1;
                } else {
                    pconf = conf;
                }
            }
            if (err_) throw std::invalid_argument("'interval' value must an integer >= 50 and <= 99");
        } else if (key == "vname") {
            // special case: here we only check if vname is a std::string
            if (!is_pystr(item.second)) throw std::invalid_argument("variable name must be a string");
            continue;
        } else {
            throw std::invalid_argument("unknown option '" + key + "'");
        }
    }
}

void GretlModel::fr_to_dict_ (FITRESID *fr, py::dict &fcdict)
{
    gretl_matrix_ptr stats{nullptr};
    py::list labels;
    double *tmp = NULL;
    double tval = 0;
    int offset, n_ok, rows, cols, t;

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
        tval = fr->asymp ? normal_critval(a2) : student_critval(fr->df, a2);
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

        tmp += rows;
        std::memcpy(tmp, fr->sderr + offset, rows * sizeof *tmp);
        tmp += rows;
        for (int i=0; i<rows; i++) {
            maxerr = tval * fr->sderr[offset + i];
            tmp[i] = fr->fitted[offset + i] - maxerr;
            tmp[i+rows] = fr->fitted[offset + i] + maxerr;
        }
        fcdict[py::cast("confidence")] = 100 * (1 - fr->alpha);
    }

    fcdict[py::cast("data")] = F;
    fcdict[py::cast("data_labels")] = labels;

    n_ok = 0;
    stats.reset(forecast_stats(fr->actual, fr->fitted, fr->t1,
                               fr->t2, &n_ok, OPT_O, &err_));
    if (stats && n_ok > 0) {
        const char **rnames = gretl_matrix_get_rownames(stats.get());
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
}

std::string GretlModel::repr_impl (void) const
{
    if (!estimated_) {
        return summary_.substr(0, summary_.size() - 1);
    }

    return summary_;
}

GretlBundlePtr GretlModel::get_bundle (void) const
{
    return GB_model_;
}

void GretlModel::check_if_estimated_ (void) const
{
    if (estimated_) throw std::runtime_error("model already estimated");
}

template <typename T>
char *GretlModel::do_save_uhat_ (T *model, int eqn,
                                 const std::string &name) const
{
    //
    // here take residuals from multivariate model
    // and add it to dataset
    // Note: eqn must be 1-based (not 0-based)
    //
    std::string uhat_name;
    gretl_double_ptr uhat{nullptr};

    // we retrive uhat
    if constexpr (std::is_same_v<T, GRETL_VAR>) {
        uhat.reset(gretl_VAR_get_resid_series(model, eqn-1, &err_));
    } else if constexpr (std::is_same_v<T, equation_system>) {
        uhat.reset(system_get_resid_series(model, eqn-1, GD_->dset, &err_));
    } else {
        static_assert(always_false<T>, "GretlModel::do_save_uhat_(): unknown type");
    }
    if (err_) handle_gretl_error(err_);

    // we set name
    if (name.empty()) {
        uhat_name = std::string("m") + std::to_string(id_)
            + std::string("_uhat") + std::to_string(eqn);
    }

    // actual work
    return GD_->add_series(std::move(uhat),
                           name.empty() ? uhat_name : name,
                           GD_->dset->n);
}

//////////////////////////////////////////////////
// single equation models
//////////////////////////////////////////////////
//
// Native Single Equation libgretl MODEL (NSE)
// extends: GretlModel
//
GretlModel_NSE::GretlModel_NSE (GretlCmdIndex ci, const py::kwargs &kwargs)
                                : GretlModel(GRETL_OBJ_EQN, ci, kwargs)
{
    model_ = gretl_model_new();
    if (!model_) handle_gretl_error(E_ALLOC);
}

GretlModel_NSE::~GretlModel_NSE (void)
{
    if (model_ != NULL) gretl_model_free(model_);
}

void *GretlModel_NSE::model_ptr_ (void) const
{
    return static_cast<void*>(model_);
}

void GretlModel_NSE::forecast (const py::kwargs &kwargs)
{
    do_forecast_<MODEL, static_cast<GretlCmdIndex>(0)>(model_, kwargs);
}

void GretlModel_NSE::test (const std::string &type,
                           const py::kwargs &kwargs)
{
    //
    // this method performs test which do not take a variables list
    //
    py::dict res;
    int order = type == "corrgm" ? auto_acf_order(sample_size(GD_->dset)) : GD_->dset->pd;

    // we set up options
    unpack_kwargs_test(opt_, type, kwargs, order);

    // we perform the test
    if (type == "bkw") {
        err_ = bkw_test(model_, GD_->dset, opt_, GP_.prn_ptr());
    } else if (type == "cusum") {
        err_ = cusum_test(model_, GD_->dset, opt_, GP_.prn_ptr());
    } else if (type == "leverage") {
        err_ = leverage_test(model_, GD_->dset,
                             opt_, GP_.prn_ptr());
    } else if (type == "panspec") {
        err_ = panel_specification_test(model_, GD_->dset,
                                        opt_, GP_.prn_ptr());
    } else if (type == "reset") {
        err_ = reset_test(model_, GD_->dset, opt_, GP_.prn_ptr());
    } else if (type == "qlrtest") {
        err_ = QLR_test(model_, GD_->dset, opt_, GP_.prn_ptr());
    } else if (type == "vif") {
        err_ = vif_test(model_, GD_->dset, opt_, GP_.prn_ptr());
    } else if (type == "corrgm") {
        err_ = corrgram(GD_->dset->v - 1, order,
                        model_->ci == ARMA ? arma_model_get_n_arma_coeffs(model_) : 0,
                        GD_->dset, SET_OPT(opt_, OPT_R), GP_.prn_ptr());
    } else if (type == "arch" || type == "autocorr" ||
               type == "breusch-pagan" || type == "comfac" ||
               type == "logs" || type == "normality" ||
               type == "panel" || type == "squares" ||
               type == "white" || type == "white-nocross" ||
               type == "xdepend" ) {
        if (get_last_model(NULL) != model_) {
            err_ = gretl_model_protect(model_);
            if (err_) handle_gretl_error(err_);
            set_as_last_model(model_, model_obj_type_);
        }
        err_ = model_test_driver(order, GD_->dset,
                                 opt_, GP_.prn_ptr());
    } else {
        throw std::invalid_argument(type + " is a dataset-based test");
    }
    if (err_) handle_gretl_error(err_);

    // we manufacture resulting object
    res[py::cast("type")] = type;
    res[py::cast("options")] = kwargs;
    res[py::cast("test")] = get_accessor("$test");
    res[py::cast("pvalue")] = get_accessor("$pvalue");
    res[py::cast("summary")] = GP_.get_buffer();

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "tests");

    if (!kwargs.contains("silent")) GP_.print();
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
        ex_var = list_from_formula(vars, GD_->dset, err_);
        if (!ex_var) throw std::invalid_argument("wrong list of variables");
    }

    // we set up options
    unpack_kwargs_test(opt_, type, kwargs, order);

    // we perform the test
    if (type == "add") {
        err_ = add_test(model_, ex_var.get(), GD_->dset,
                        opt_, GP_.prn_ptr());
    } else if (type == "chow") {
        err_ = chow_test_driver(vars.c_str(), model_, GD_->dset,
                                opt_, GP_.prn_ptr());
    } else if (type == "coeffsum") {
        err_ = gretl_sum_test(ex_var.get(), model_, GD_->dset,
                              opt_, GP_.prn_ptr());
    } else if (type == "omit") {
        err_ = omit_test(model_, ex_var.get(), GD_->dset,
                         opt_, GP_.prn_ptr());
    } else {
        throw std::invalid_argument(type + " is a dataset-based test");
    }
    if (err_) handle_gretl_error(err_);

    // we manufacture resulting object
    res[py::cast("type")] = type;
    res[py::cast("options")] = kwargs;
    res[py::cast("test")] = get_accessor("$test");
    res[py::cast("pvalue")] = get_accessor("$pvalue");
    res[py::cast("summary")] = GP_.get_buffer();

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "tests");

    if (!kwargs.contains("silent")) GP_.print();
}

void GretlModel_NSE::restrict (const py::list &impose,
                               const py::kwargs &kwargs)
{
    do_restrict_<MODEL>(model_, impose, kwargs);
}

template <ModelDataIndex IDX>
char *GretlModel_NSE::gretl_get_fit_or_resid (const std::string &name) const
{
    //
    // we save yhat, uhat, uhat^2, ahat, and h
    // Note: we return name of newly added series
    //
    char tmp_name[VNAMELEN] = {0};
    gchar *pdesc_raw = NULL;

    gretl_double_ptr data(get_fit_or_resid(model_, GD_->dset, IDX,
                                           tmp_name, &pdesc_raw, &err_));
    if (err_) handle_gretl_error(err_);

    // we save name and description
    std::string vname = name.empty() ? std::string(tmp_name) : name;
    gretl_gchar_ptr pdesc(pdesc_raw);

    return GD_->add_series(std::move(data), vname,
                           GD_->dset->n, std::move(pdesc));
}
template char *GretlModel_NSE::gretl_get_fit_or_resid<M_YHAT>(const std::string&) const;
template char *GretlModel_NSE::gretl_get_fit_or_resid<M_UHAT>(const std::string&) const;
template char *GretlModel_NSE::gretl_get_fit_or_resid<M_UHAT2>(const std::string&) const;
template char *GretlModel_NSE::gretl_get_fit_or_resid<M_AHAT>(const std::string&) const;
template char *GretlModel_NSE::gretl_get_fit_or_resid<M_H>(const std::string&) const;

double GretlModel_NSE::adj_rsq (void) const
{
    return model_->adjrsq;
}

template <GretlCmdIndex CI>
void GretlModel_NSE::regexp_extract_ (const std::string &formula,
                                      std::string &formula_left,
                                      std::string &formula_right)
{
    //
    // method for extraction model formula from regular expression
    //
    std::regex formula_regex;

    // We extract GMM(...) or mds(l) blocks into formula_right, and remove them from formula_left
    if constexpr (CI == DPANEL) {
        formula_regex = std::regex(R"(GMM\s*\([^()]*\))");
    } else if constexpr (CI == MIDASREG) {
        formula_regex = std::regex(R"((mds|mdsl)\s*\([^()]*\))");
    }

    std::sregex_iterator begin(formula.begin(), formula.end(), formula_regex);
    std::sregex_iterator end;
    formula_left = formula;
    formula_right.clear();

    for (auto it = begin; it != end; ++it) {
        formula_right += it->str() + " ";
        formula_left.erase(formula_left.find(it->str()), it->str().length());
    }
    gretl4py_trim(formula_left);
    gretl4py_trim(formula_right);
    if (formula_left.back() == ';') {
        formula_left.pop_back();
    }
}

//
// NSE via arima plugin
//
GretlModel_NSE_arima::GretlModel_NSE_arima (const std::string &y,
                                            const py::tuple &tup,
                                            const py::kwargs &kwargs)
                                            : GretlModel_NSE(ARMA, kwargs),
                                            y_(y)
{
    // lambda for ARIMA order
    auto get_arima_order = [](const py::tuple &tup, int &p, int &d, int &q)
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
    };

    // we set up "y"
    y_number_ = this->GD_->get_series_id(y, false);
    if (y_number_ == -1) throw std::invalid_argument("There is no variable: " + y);

    // we set up non-seasonal parameters
    p_ = d_ = q_ = 0;
    get_arima_order(tup, p_, d_, q_);

    // we set up seasonal parameters
    if (kwargs.contains("seasonal") && !kwargs["seasonal"].is_none()) {
        const py::tuple seas = kwargs["seasonal"].cast<py::tuple>();
        P_ = D_ = Q_ = 0;

        get_arima_order(seas, P_, D_, Q_);
        if (P_ > 0 || D_ > 0 || Q_ > 0) {
            seasonal_ = 1;
        }
    }

    // we add exogenous variables if present
    if (kwargs.contains("xlist") && !kwargs["xlist"].is_none()) {
        xlist_ = list_from_formula(kwargs["xlist"].cast<std::string>(),
                                   this->GD_->dset, err_);
        if (!xlist_) {
            throw std::invalid_argument("wrong list of exogenous variables");
        } else if (err_) {
            handle_gretl_error(err_);
        }
        exogenous_ = xlist_.get()[0];
    }
}

GretlModel_NSE_arima::~GretlModel_NSE_arima (void) = default;

std::string GretlModel_NSE_arima::get_formula (void) const
{
    return y_;
}

void GretlModel_NSE_arima::fit (void)
{
    int i = 1;

    // we check if we deal with clean model
    check_if_estimated_();

    var_list_.reset(gretl_list_new(seasonal_ ? 9+exogenous_ : 5+exogenous_));
    if (!var_list_) handle_gretl_error(E_ALLOC);

    var_list_.get()[i++] = p_;
    var_list_.get()[i++] = d_;
    var_list_.get()[i++] = q_;
    if (seasonal_) {
        var_list_.get()[i++] = LISTSEP;
        var_list_.get()[i++] = P_;
        var_list_.get()[i++] = D_;
        var_list_.get()[i++] = Q_;
    }
    var_list_.get()[i++] = LISTSEP;
    var_list_.get()[i++] = y_number_;
    if (exogenous_) {
        for (int j=1; j<=exogenous_; j++) {
            var_list_.get()[i++] = xlist_.get()[j];
        }
    }

    // do the job
    *model_ = arma(var_list_.get(), NULL, GD_->dset,
                   opt_, GP_.prn_ptr());

    // save model
    save_model_<MODEL>(model_);
}

//
// NSE via dedicated Driver
//
template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver<F, CI>::GretlModel_NSE_Driver (const std::string &formula,
                                                     const py::kwargs &kwargs)
                                                     : GretlModel_NSE(CI, kwargs),
                                                     formula_(formula)
{}

template <auto F, GretlCmdIndex CI>
std::string GretlModel_NSE_Driver<F, CI>::get_formula (void) const
{
    return formula_;
}

// Method template to support several estimation methods that
// have the same "basic" signature, plus a few special cases
// F: function to call
// CI: GretlCmdIndex
template <auto F, GretlCmdIndex CI>
void GretlModel_NSE_Driver<F, CI>::fit (void)
{
    std::string formula_right = "";

    // we check if we deal with clean model
    check_if_estimated_();

    // we construct the model
    if constexpr (CI == MIDASREG) {
        std::string formula_left;

        regexp_extract_<CI>(formula_, formula_left, formula_right);
        var_list_ = list_from_formula(formula_left, GD_->dset, err_);
    } else {
        var_list_ = list_from_formula(formula_, GD_->dset, err_);
    }
    if (!var_list_) throw std::runtime_error("Formula rendering failed");

    // we call F as an estimator
    if constexpr (use_ldco_sig(CI)) {
        // fallback to Pooled OLS if needed
        if (CI == OLS && GD_->is_panel()) {
            SET_OPT(opt_, (OPT_P | OPT_L));
            *model_ = panel_model(var_list_.get(), GD_->dset,
                                  opt_, GP_.prn_ptr());
        } else {
            *model_ = F(var_list_.get(), GD_->dset, CI, opt_);
        }
    } else if constexpr (use_ldcop_sig(CI)) {
        *model_ = F(var_list_.get(), GD_->dset,
                    static_cast<int>(CI), opt_, GP_.prn_ptr());
    } else if constexpr (use_ldo_sig(CI)) {
        *model_ = F(var_list_.get(), GD_->dset, opt_);
    } else if constexpr (use_ldop_sig(CI)) {
        *model_ = F(var_list_.get(), GD_->dset,
                    opt_, GP_.prn_ptr());
    } else if constexpr (use_lcdop_sig(CI)) {
        *model_ = F(var_list_.get(), static_cast<int>(CI),
                    GD_->dset, opt_, GP_.prn_ptr());
    } else if constexpr (CI == MIDASREG) {
        *model_ = F(var_list_.get(), formula_right.c_str(),
                    GD_->dset, opt_, GP_.prn_ptr());
    }

    // save results
    save_model_<MODEL>(model_);
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
template class GretlModel_NSE_Driver<hsk_model, HSK>;
template class GretlModel_NSE_Driver<interval_model, INTREG>;
template class GretlModel_NSE_Driver<midas_model, MIDASREG>;

//
// NSE_Driver_quantreg via quantreg_driver
// extends: GretlModel_NSE_Driver
//
GretlModel_NSE_Driver_quantreg::GretlModel_NSE_Driver_quantreg
                                    (const std::string &formula,
                                     const double &tau,
                                     const py::kwargs &kwargs)
                                     : GretlModel_NSE_Driver<quantreg_driver, QUANTREG>(formula, kwargs),
                                     tau_(std::to_string(tau))
{}

GretlModel_NSE_Driver_quantreg::GretlModel_NSE_Driver_quantreg
                                    (const std::string &formula,
                                     const py::array_t<double> &tau,
                                     const py::kwargs &kwargs)
                                     : GretlModel_NSE_Driver<quantreg_driver, QUANTREG>(formula, kwargs)
{
    if (tau.ndim() != 1) throw std::invalid_argument("'tau' is expected to be a vector");

    tau_ = "{";
    for (int i=0; i<tau.shape(0); ++i) {
        tau_.append(std::to_string(tau.at(i)));
        if (i < tau.shape(0)-1) {
            tau_.append(";");
        }
    }
    tau_.append("}");
}

void GretlModel_NSE_Driver_quantreg::fit (void)
{
    // we check if we deal with clean model
    check_if_estimated_();

    // we define list of variables
    var_list_ = list_from_formula(formula_, GD_->dset, err_);
    if (!var_list_) throw std::runtime_error("Formula rendering failed");

    // we do the job
    *model_ = quantreg_driver(tau_.c_str(), var_list_.get(),
                              GD_->dset, opt_, GP_.prn_ptr());

    // we save results
    save_model_<MODEL>(model_);
}

//
// NSE_Driver_dpanel via dpd_model
// extends: GretlModel_NSE_Driver
//
GretlModel_NSE_Driver_dpanel::GretlModel_NSE_Driver_dpanel
                                        (const std::string &formula,
                                         const int &p, const py::kwargs &kwargs)
                                        : GretlModel_NSE_Driver<dpd_model, DPANEL>(formula, kwargs)
{
    lags_.push_back(p);
}

GretlModel_NSE_Driver_dpanel::GretlModel_NSE_Driver_dpanel
                                        (const std::string &formula,
                                         const py::array_t<int> &p,
                                         const py::kwargs &kwargs)
                                        : GretlModel_NSE_Driver<dpd_model, DPANEL>(formula, kwargs)
{
    if (p.ndim() != 1) throw std::invalid_argument("'p' is expected to be a vector");

    for (int i=0; i<p.shape(0); ++i) {
        lags_.push_back(static_cast<double>(p.at(i)));
    }
}

void GretlModel_NSE_Driver_dpanel::fit (void)
{
    gretl_list_ptr dpanel_list{nullptr};
    gretl_list_ptr laglist{nullptr};
    std::string formula_left;
    std::string formula_right = "";

    // we check if we deal with clean model
    check_if_estimated_();

    // we define list of variables
    regexp_extract_<DPANEL>(formula_, formula_left, formula_right);
    var_list_ = list_from_formula(formula_left, GD_->dset, err_);
    if (!var_list_) throw std::runtime_error("Formula rendering failed");

    // we setup dpanel_list: lags + var_list
    dpanel_list.reset(gretl_list_new(var_list_.get()[0] + 2));
    if (lags_.size() == 1) {
        dpanel_list.get()[1] = static_cast<int>(lags_.at(0));
    } else {
        dpanel_list.get()[1] = static_cast<int>(*std::max_element(lags_.begin(),
                                                                  lags_.end()));
    }
    dpanel_list.get()[2] = LISTSEP;
    std::memcpy(dpanel_list.get() + 3, var_list_.get() + 1,
                var_list_.get()[0] * sizeof(int));

    // we do the job
    if (lags_.size() > 1) {
        laglist.reset(gretl_list_new(lags_.size()));
        for (size_t i=0; i<lags_.size(); i++) {
            laglist.get()[i+1] = static_cast<int>(lags_[i]);
        }
    }
    *model_ = dpd_model(dpanel_list.get(), laglist.get(),
                        formula_right.c_str(), GD_->dset,
                        opt_, GP_.prn_ptr());

    // we save results
    save_model_<MODEL>(model_);
}

//
// NSE_Driver_tobit via tobit_model
// extends: GretlModel_NSE_Driver
//
GretlModel_NSE_Driver_tobit::GretlModel_NSE_Driver_tobit
                                        (const std::string &formula, const py::kwargs &kwargs)
                                         : GretlModel_NSE_Driver<tobit_model, TOBIT>(formula, kwargs)
{
    // we set (l/r)limit
    if (kwargs.contains("llimit") && !kwargs["llimit"].is_none()) {
        llimit_ = py::cast<double>(kwargs["llimit"]);
    }

    if (kwargs.contains("rlimit") && !kwargs["rlimit"].is_none()) {
        rlimit_ = py::cast<double>(kwargs["rlimit"]);
    }
}

void GretlModel_NSE_Driver_tobit::fit (void)
{
    // we check if we deal with clean model
    check_if_estimated_();

    // we define list of variables
    var_list_ = list_from_formula(formula_, GD_->dset, err_);
    if (!var_list_) throw std::runtime_error("Formula rendering failed");

    // we do the job
    *model_ = tobit_model(var_list_.get(), llimit_, rlimit_,
                          GD_->dset, opt_, GP_.prn_ptr());

    // we save results
    save_model_<MODEL>(model_);
}

//
// NLS,MLE,GMM via plugin
// extends: GretlModel_NSE
//
template <GretlCmdIndex CI>
GretlModel_NSE_nl<CI>::GretlModel_NSE_nl (const std::string formula,
                                          const py::kwargs &kwargs)
                                          : GretlModel_NSE(CI, kwargs),
                                          command_(gretl_command_word(CI)),
                                          formula_(formula)
{
    // we get model definition
    load_objects_from_dict<std::string>(kwargs, "params", params_);
    load_objects_from_dict<std::string>(kwargs, "param_names", param_names_);
    if (!param_names_.empty()) {
        param_names_ = "\"" + param_names_ + "\"";
    }

    // we set up initial values
    if (kwargs.contains("initvals")) {
        if (!is_pydict(kwargs["initvals"])) throw std::invalid_argument("'initvals' must be a dictionary");
        py::dict initvals = kwargs["initvals"].cast<py::dict>();

        for (const auto &[key, val] : initvals) {
            gretl_generate<GRETL_TYPE_ANY>(py::str(val), GD_,
                                           py::cast<std::string>(key));
        }
    }

    // special cases
    if constexpr (CI == NLS || CI == MLE) {
        // NLS and MLE special case: we get derivatives
        if (kwargs.contains("deriv")) {
            auto deriv = kwargs["deriv"];

            if (is_pydict(deriv)) {
                for (const auto &[key, val] : deriv.cast<py::dict>()) {
                    deriv_.push_back(py::cast<std::string>(key) + " = "
                                     + py::str(val).cast<std::string>());
                }
            } else if (is_pylist(deriv)) {
                deriv_ = py::cast<std::vector<std::string>>(deriv);
            } else {
                throw std::invalid_argument("'deriv' must be either a list or dict");
            }
        }
    } else if constexpr (CI == GMM) {
        load_objects_from_dict<std::vector<std::string>>(kwargs, "orthog", orthog_);
        load_objects_from_dict<std::string>(kwargs, "weights", weights_);
    }

    // we get statements entries
    if constexpr (CI == MLE || CI == GMM) {
        load_objects_from_dict<std::vector<std::string>>(kwargs, "statements", statements_);
    }
}

template <GretlCmdIndex CI>
GretlModel_NSE_nl<CI>::~GretlModel_NSE_nl (void) = default;

template <GretlCmdIndex CI>
template <typename T>
void GretlModel_NSE_nl<CI>::do_nl_parse_line_ (const std::string &keyword,
                                               T *src)
{
    //
    // here we perform line parsing and adding (to libgretl)
    // Note: to add formula, use "command" as keyword and nullptr as src
    //

    // argument validation:
    // 1. keyword must not be empty
    // 2. if keyword != "command", src must not be null
    if (keyword.empty() || (keyword != "command" && !src)) {
        throw std::invalid_argument("do_nl_parse_line_(): wrong arguments passed");
    }

    // we do the job
    if constexpr (std::is_same_v<T, std::string>) {
        std::string line;

        if (keyword == "command") {
            line = command_ + " " + formula_;
        } else {
            if (src->empty()) return;
            line = keyword;
            line.append(" ");
            line.append(*src);
        }
        err_ = nl_parse_line(CI, line.c_str(), GD_->dset, GP_.prn_ptr());
        if (err_) handle_gretl_error(err_);
    } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
        if (!src->size()) return;

        std::string line;

        for (const auto &el : *src) {
            if (keyword == "statements") {
                line = el;
            } else {
                line = keyword;
                line.append(" ");
                line.append(el);
            }
            err_ = nl_parse_line(CI, line.c_str(),
                                 GD_->dset, GP_.prn_ptr());
            if (err_) handle_gretl_error(err_);
        }
    } else {
        static_assert(always_false<T>, "GretlModel_NSE_nl::do_nl_parse_line_(): unknown type");
    }
}

template <GretlCmdIndex CI>
void GretlModel_NSE_nl<CI>::fit (void)
{
    // we check if we deal with clean model
    check_if_estimated_();

    // we set up model (nls, mle, gmm)
    do_nl_parse_line_<std::string>("command");

    // we set up all model definition lines
    do_nl_parse_line_<std::string>("params", &params_);
    do_nl_parse_line_<std::string>("param_names", &param_names_);
    if constexpr (CI == NLS || CI == MLE) {
        do_nl_parse_line_<std::vector<std::string>>("deriv", &deriv_);
    }
    if constexpr (CI == GMM) {
        do_nl_parse_line_<std::vector<std::string>>("orthog", &orthog_);
        do_nl_parse_line_<std::string>("weights", &weights_);
    }
    if constexpr (CI == MLE || CI == GMM) {
        do_nl_parse_line_<std::vector<std::string>>("statements", &statements_);
    }

    // we do the job
    *model_ = nl_model(GD_->dset, opt_, GP_.prn_ptr());

    // we save results
    save_model_<MODEL>(model_);
}

template <GretlCmdIndex CI>
std::string GretlModel_NSE_nl<CI>::get_formula (void) const
{
    std::string RES = formula_ + "\n";

    if constexpr (CI == NLS || CI == MLE) {
        if (deriv_.size()) {
            for (const auto &el : deriv_) {
                RES += "deriv " + el + "\n";
            }
        }
    }
    if constexpr (CI == GMM) {
        if (orthog_.size()) {
            for (const auto &el : orthog_) {
                RES += "orthog " + el + "\n";
            }
        }

        if (!weights_.empty()) {
            RES += "weights " + weights_ + "\n";
        }
    }
    if constexpr (CI == MLE || CI == GMM) {
        if (statements_.size()) {
            for (const auto &el : statements_) {
                RES += el + "\n";
            }
        }
    }
    if (!params_.empty()) {
        RES += "params " + params_ + "\n";
    }
    if (!param_names_.empty()) {
        RES += "param_names " + param_names_ + "\n";
    }

    return RES;
}

template <GretlCmdIndex CI>
template <bool ForString>
void GretlModel_NSE_nl<CI>::add_line_to_parse (const std::string &keyword,
                                               const std::string &val)
{
    //
    // method set/insert a val containing a line to parse
    //

    if constexpr (ForString) {
        if (keyword == "params") {
            params_ = val;
        } else if (keyword == "param_names") {
            param_names_ = "\"";
            param_names_.append(val);
            param_names_.append("\"");
        } else if (keyword == "weights") {
            weights_ = val;
        } else {
            throw std::invalid_argument("unknown keyword: " + keyword);
        }
    } else {
        if (keyword == "deriv") {
            deriv_.push_back(val);
        } else if (keyword == "orthog") {
            orthog_.push_back(val);
        } else if (keyword == "statements") {
            statements_.push_back(val);
        } else {
            throw std::invalid_argument("unknown keyword: " + keyword);
        }
    }
}

template <GretlCmdIndex CI>
void GretlModel_NSE_nl<CI>::add_init_value (const std::string &varname,
                                            double value)
{
    //
    // adds/sets value of varname in the contexts of GD
    //

    gretl_generate<GRETL_TYPE_ANY>(std::to_string(value), GD_, varname);
}

// explicit instantiations for all CI (including methods)
template class GretlModel_NSE_nl<NLS>;
template void GretlModel_NSE_nl<NLS>::add_line_to_parse<true>(const std::string&, const std::string&);
template void GretlModel_NSE_nl<NLS>::add_line_to_parse<false>(const std::string&, const std::string&);
template class GretlModel_NSE_nl<MLE>;
template void GretlModel_NSE_nl<MLE>::add_line_to_parse<true>(const std::string&, const std::string&);
template void GretlModel_NSE_nl<MLE>::add_line_to_parse<false>(const std::string&, const std::string&);
template class GretlModel_NSE_nl<GMM>;
template void GretlModel_NSE_nl<GMM>::add_line_to_parse<true>(const std::string&, const std::string&);
template void GretlModel_NSE_nl<GMM>::add_line_to_parse<false>(const std::string&, const std::string&);

//
// VAR libgretl MODEL
// extends: GretlModel
//
GretlModel_VAR::GretlModel_VAR (const std::string &ylist,
                                const int order, GretlCmdIndex ci,
                                const py::kwargs &kwargs)
                                : GretlModel(GRETL_OBJ_VAR, ci, kwargs),
                                vars_(ylist), order_(order)
{
    if (kwargs.contains("xlist") && !kwargs["xlist"].is_none()) {
        vars_.append(" ; ");
        vars_.append(kwargs["xlist"].cast<std::string>());
    }
}

GretlModel_VAR::~GretlModel_VAR (void)
{
    if (this->model_ != NULL) gretl_VAR_free(this->model_);
}

void *GretlModel_VAR::model_ptr_ (void) const
{
    return static_cast<void*>(model_);
}

// true VAR/VECM estimate
template <GretlCmdIndex CI>
void GretlModel_VAR::do_var_vecm_fit_ (const int coint_rank)
{
    //
    // here we perform a real VAR/VECM estimation
    // Note: for VAR case we do not use rank, of course
    //
    // we check if we deal with clean model
    check_if_estimated_();

    // be build variables list
    var_list_ = list_from_formula(vars_, GD_->dset, err_);
    if (!var_list_) throw std::invalid_argument("wrong list of variables");

    // we estimate a model
    if constexpr (CI == VAR) {
        model_ = gretl_VAR(order_, NULL, var_list_.get(),
                           GD_->dset, opt_, GP_.prn_ptr(), &err_);
    } else if constexpr (CI == VECM) {
        model_ = gretl_VECM(order_, coint_rank, var_list_.get(),
                            GD_->dset, opt_, GP_.prn_ptr(), &err_);
    }
    if (err_) handle_gretl_error(err_);

    // save results
    save_model_<GRETL_VAR>(model_);
}

// true VAR/VECM forecasting
template <GretlCmdIndex CI>
void GretlModel_VAR::do_var_vecm_forecast_ (const py::kwargs &kwargs)
{
    int variable_nr = -2;   // print for all equations

    // we check if we forecast for all variables or just one
    if (kwargs.contains("vname") && !kwargs["vname"].is_none()) {
        variable_nr = find_variable_nr_(py::cast<std::string>(kwargs["vname"]));
        if (variable_nr == -1) throw std::invalid_argument("unknown variable '"
                                                           + py::cast<std::string>(kwargs["vname"])
                                                           + "'");
    }

    // actual work
    for (int i=0; i<model_->neqns; i++) {
        if (variable_nr == -2 || variable_nr == i) {
            py::print("Equation ", i+1);
            do_forecast_<GRETL_VAR, CI>(model_, kwargs, i);
        }
    }
}

void GretlModel_VAR::fit (void)
{
    do_var_vecm_fit_<VAR>();
}

void GretlModel_VAR::forecast (const py::kwargs &kwargs)
{
    do_var_vecm_forecast_<VAR>(kwargs);
}

void GretlModel_VAR::test (const std::string &type, const py::kwargs &kwargs)
{
    py::dict res;
    int order = GD_->dset->pd;

    // basic check
    if (type == "normality" && kwargs.contains("order")) throw std::invalid_argument("normality test doesn't take 'order' option");

    // we set up options
    unpack_kwargs_test(opt_, type, kwargs, order);

    // we perform test
    if (type == "autocorr") {
        err_ = gretl_VAR_autocorrelation_test(model_, order, GD_->dset,
                                              opt_, GP_.prn_ptr());
    } else if (type == "arch") {
        err_ = gretl_VAR_arch_test(model_, order, GD_->dset,
                                   opt_, GP_.prn_ptr());
    } else if (type == "normality") {
        err_ = gretl_VAR_normality_test(model_, opt_, GP_.prn_ptr());
    } else {
        throw std::invalid_argument(type + ": unsupported test");
    }
    if (err_) handle_gretl_error(err_);

     // we manufacture resulting object
    res[py::cast("type")] = type;
    res[py::cast("options")] = kwargs;
    res[py::cast("test")] = get_accessor("$test");
    res[py::cast("pvalue")] = get_accessor("$pvalue");
    res[py::cast("summary")] = GP_.get_buffer();

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "tests");

    if (!kwargs.contains("silent")) GP_.print();
}

std::string GretlModel_VAR::get_formula (void) const
{
    return vars_;
}

char *GretlModel_VAR::save_uhat (int eqn, const std::string &name) const
{
    return do_save_uhat_<GRETL_VAR>(model_, eqn, name);
}

int GretlModel_VAR::find_variable_nr_ (const std::string &variable_name)
{
    //
    // here we find the number of given variable in the context of VAR/VECM
    //
    std::istringstream iss(vars_);
    std::string token;
    int pos = 0;

    while (iss >> token) {
        if (token == ";") {
            break;
        }
        if (token == variable_name) {
            return pos;
        }
        ++pos;
    }

    return -1; // not found
}

void GretlModel_VAR::irf (int targ, int shock, double alpha)
{
    //
    // this method mimics gretl's irf() function
    //
    py::dict res;

    // we compute IRFs
    gretl_matrix_ptr irf(gretl_VAR_get_impulse_response(model_, targ, shock,
                                                        default_VAR_horizon(GD_->dset),
                                                        alpha, GD_->dset, &err_));
    if (err_) handle_gretl_error(err_);

    // we manufacture resulting object
    res[py::cast("targ")] = targ;
    res[py::cast("targ_name")] = GD_->dset->varname[gretl_VAR_get_variable_number(model_, targ)];
    res[py::cast("shock")] = shock;
    res[py::cast("shock_name")] = GD_->dset->varname[gretl_VAR_get_variable_number(model_, shock)];
    if (alpha > 0.0) {
        res[py::cast("alpha")] = alpha;
    }

    auto val = convert_GretlType_to_GretlTypes(GRETL_TYPE_MATRIX, irf.get(), 0, err_);
    if (err_) handle_gretl_error(err_);
    res[py::cast("results")] = variant_to_pyobject(std::move(*val));

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, std::move(res), "irfs");
}

void GretlModel_VAR::irf (const std::string &targ, const std::string &shock, double alpha)
{
    //
    // this method mimics gretl's irf() function
    // Note: overload which takes names instead of equation number
    //
    int targ_int;
    int shock_int;

    // let's try to find our targ/shock IDs
    try {
        targ_int = std::stoi(targ);
    } catch (std::invalid_argument &e) {
        targ_int = find_variable_nr_(targ);
    }
    try {
        shock_int = std::stoi(shock);
    } catch (std::invalid_argument &e) {
        shock_int = find_variable_nr_(shock);
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
GretlModel_VAR_VECM::GretlModel_VAR_VECM (const std::string &ylist, const int order,
                                          const int rank, const py::kwargs &kwargs)
                                          : GretlModel_VAR(ylist, order, VECM, kwargs),
                                          coint_rank_(rank)
{
    if (kwargs.contains("rxlist") && !kwargs["rxlist"].is_none()) {
        vars_.append(" ; ");
        vars_.append(kwargs["rxlist"].cast<std::string>());
    }
}

void GretlModel_VAR_VECM::fit (void)
{
    do_var_vecm_fit_<VECM>(coint_rank_);
}

void GretlModel_VAR_VECM::restrict (const py::list &impose,
                                    const py::kwargs &kwargs)
{
    do_restrict_<GRETL_VAR>(model_, impose, kwargs);
}

char *GretlModel_VAR_VECM::save_ec_term (const int nr,
                                         const std::string &name) const
{
    //
    // here we add EC to dataset of number nr
    // Note: nr must be 1-based (not 0-based)
    //
    std::string ec_term_name;

    // we retrive ec_term
    gretl_double_ptr ec_term(gretl_VECM_get_EC(model_, nr-1,
                                               GD_->dset, &err_));
    if (err_) handle_gretl_error(err_);

    // we set name
    if (name.empty()) {
        ec_term_name = std::string("m") + std::to_string(id_)
            + std::string("_EC") + std::to_string(nr);
    }

    // actual work
    return GD_->add_series(std::move(ec_term),
                           name.empty() ? ec_term_name : name,
                           GD_->dset->n);
}

void GretlModel_VAR_VECM::forecast (const py::kwargs &kwargs)
{
    do_var_vecm_forecast_<VECM>(kwargs);
}

//
// Native Multiple Equation libgretl SYSTEM (NME)
// extends: GretlModel
//
GretlModel_NME::GretlModel_NME (const std::string &method,
                                const py::kwargs &kwargs)
                                : GretlModel(GRETL_OBJ_SYS, SYSTEM, kwargs),
                                method_(method)
{
    // we load system definition
    load_objects_from_dict<std::vector<std::string>>(kwargs, "equations", equations_);
    if (equations_.empty()) {
        throw std::runtime_error("no equations definitions passed");
    }
    load_objects_from_dict<std::vector<std::string>>(kwargs, "instruments", instruments_);
    load_objects_from_dict<std::vector<std::string>>(kwargs, "identities", identities_);
    load_objects_from_dict<std::string>(kwargs, "endogs", endogs_);
}

GretlModel_NME::~GretlModel_NME (void)
{
    if (this->model_ != NULL) equation_system_destroy(this->model_);
}

void *GretlModel_NME::model_ptr_ (void) const
{
    return static_cast<void*>(model_);
}

void GretlModel_NME::fit (void)
{
    //lambda for setting instruments_, identities_ and endogs
    auto do_system_parse_line = [&](const char *type,
                                    std::vector<std::string> &src)
    {
        if (src.empty()) return;

        std::string line;

        for (const auto &el : src) {
            line = std::string(type);
            line.append(" ");
            line.append(el);
            err_ = system_parse_line(model_, line.c_str(), GD_->dset);
            if (err_) handle_gretl_error(err_);
        }
    };

    // we check if we deal with clean model
    check_if_estimated_();

    // system definition
    model_ = equation_system_start(std::string("method=").append(method_).data(),
                                   NULL, opt_, &err_);
    if (err_) handle_gretl_error(err_);

    // we define equations
    for (const auto &el : equations_) {
        var_list_ = list_from_formula(el, GD_->dset, err_);
        if (!var_list_) throw std::runtime_error("Formula rendering failed");
        err_ = equation_system_append(model_, var_list_.get());
        if (err_) handle_gretl_error(err_);
    }

    // we define the rest of the system
    do_system_parse_line("instr", instruments_);
    do_system_parse_line("identity", identities_);
    if (endogs_.size()) {
        std::string line = "endog " + endogs_;
        err_ = system_parse_line(model_, line.c_str(), GD_->dset);
        if (err_) handle_gretl_error(err_);
    }

    // do the job
    err_ = equation_system_finalize(model_, GD_->dset,
                                   opt_, GP_.prn_ptr());
    if (err_) handle_gretl_error(err_);

    // we save model
    save_model_<equation_system>(model_);
}

std::string GretlModel_NME::get_formula (void) const
{
    py::print("GretlModel_NME::get_formula(): to be written");

    return "";
}

void GretlModel_NME::forecast (const py::kwargs &kwargs)
{
    py::print("GretlModel_NME::forecast(): to be written");
}

void GretlModel_NME::restrict (const py::list &impose,
                               const py::kwargs &kwargs)
{
    do_restrict_<equation_system>(model_, impose, kwargs);
}

char *GretlModel_NME::save_uhat (int eqn, const std::string &name) const
{
    return do_save_uhat_<equation_system>(model_, eqn, name);
}

//
// libgretl contributed packages
// extends: GretlModel
//
GretlModel_package::GretlModel_package (const std::string &name,
                                        const py::kwargs &kwargs)
                                        : GretlModel(GRETL_OBJ_ANY, NC, kwargs),
                                        kwargs_(kwargs)
{
    std::string basename;
    std::string suffix = "";

    // we check if we deal with suffix-ed string or just basename
    if (has_suffix(name.c_str(), ".gfn") || has_suffix(name.c_str(), ".zip")) {
        size_t dot_pos = name.find('.');

        basename = name.substr(0, dot_pos);
        suffix = name.substr(dot_pos);
    } else {
        basename = name;
    }

    // we load the package
    GPKG_ = load_package(basename, suffix);

    // we setup some internal parameters
    main_func_name_ = GPKG_->main_func_get_name();

    if (!GPKG_->is_estimator()) throw std::runtime_error(GPKG_->get_name() + " package doesn't provide an estimation method");
    if (main_func_name_.empty()) throw std::runtime_error("no main function in package: " + GPKG_->get_name());

    // we register special functions (if exists)
    print_func_name_ = GPKG_->get_function_with_role(UFUN_BUNDLE_PRINT);
    fcast_func_name_ = GPKG_->get_function_with_role(UFUN_BUNDLE_FCAST);
}

void *GretlModel_package::model_ptr_ (void) const
{
    return static_cast<void*>(nullptr);
}

void GretlModel_package::fit (void)
{
    res_ = GPKG_->function_exec(main_func_name_, GD_, GP_, kwargs_);

    // saving results
    py::object self = py::cast(this);

    if (res_.type == GRETL_TYPE_BUNDLE) {
        GB_model_ = std::make_shared<GretlBundle>(GretlBundle(static_cast<gretl_bundle*>(res_.get())));
        GB_model_->python_register(self);
    } else {
        auto val = convert_GretlType_to_GretlTypes(res_.type, res_.get(), 0, err_);

        if (err_) handle_gretl_error(err_);
        py::setattr(self, "results", variant_to_pyobject(std::move(*val)));
    }
    summary_ = GP_.get_buffer();
}

void GretlModel_package::forecast (const py::kwargs &kwargs)
{
    py::print("GretlModel_package::forecast");
}

std::string GretlModel_package::get_formula (void) const
{
    return "empty";
}

void GretlModel_package::functions (void) const
{
    GPKG_->show_functions();
}
