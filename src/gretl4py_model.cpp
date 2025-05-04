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
extern "C" {
    #include <gretl/gretl_restrict.h>
}
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
GretlModel::GretlModel (std::shared_ptr<GretlDataset> GD)
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

void GretlModel::unpack_kwargs (const py::kwargs &kwargs, GretlCmdIndex ci)
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
            // arima special: ignore here
            continue;
        } else if (key == "lagselect" && ci == ARMA) {
            throw std::invalid_argument("'lagselect' option for (S)ARIMA is temporarily unavailable");
        } else if ((key == "llimit" || key == "rlimit") && ci == TOBIT) {
            // TOBIT special: we set (l/r)limit
            if (!is_pynumeric(item.second)) {
                throw std::invalid_argument("both llimit and rlimit must be numeric");
            } else {
                if (key == "llimit") {
                    optional_par.at(0) = item.second.cast<double>();
                } else {
                    optional_par.at(1) = item.second.cast<double>();
                }
            }
            continue;
        } else {
            gretl_valid_long_opt(opt, ci, key, item.second);
        }
    }
}

void GretlModel::unpack_kwargs_forecast (const py::kwargs &kwargs)
{
    opt = OPT_NONE;

    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::invalid_argument("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "quiet") {
            opt = static_cast<gretlopt>(opt | OPT_Q);
        }
        //std::cout << "the following pairs of forecasting options were passed:\n";
        //std::cout << item.first.cast<std::string>() << " : ";
        //std::cout << MapPyToCpp(item.second).to_string_view() << std::endl;
    }
}

void GretlModel::fr_to_dict (FITRESID *fr, py::dict &fcdict)
{
    gretl_matrix *stats = NULL;
    double *tmp;
    int offset;
    int n_ok;
    int rows;
    int t;

    rows = fr->t2 - fr->t1 + 1;
    offset = fr->nobs - rows;

    fcdict[py::cast("t1")] = fr->t1 + 1;
    fcdict[py::cast("t2")] = fr->t2 + 1;

    /* count of non-missing actual values */
    n_ok = 0;
    for (t=fr->t1; t<=fr->t2; t++) {
        if (!na(fr->actual[t])) {
            n_ok++;
        }
    }

    /* actual values? */
    if (n_ok > 0) {
        DNumPyArrayFC A(rows);
        tmp = A.mutable_data();
        std::memcpy(tmp, fr->actual + offset, rows * sizeof *tmp);
        fcdict[py::cast("actual")] = A;
    }

    /* predicted values */
    DNumPyArrayFC P(rows);
    tmp = P.mutable_data();
    std::memcpy(tmp, fr->fitted + offset, rows * sizeof *tmp);
    fcdict[py::cast("predicted")] = P;

    /* standard errors? */
    if (fr->sderr != NULL) {
        DNumPyArrayFC S(rows);
        tmp = S.mutable_data();
        std::memcpy(tmp, fr->sderr + offset, rows * sizeof *tmp);
        fcdict[py::cast("stderr")] = S;
    }

    n_ok = 0;
    stats = forecast_stats(fr->actual, fr->fitted, fr->t1, fr->t2,
                           &n_ok, OPT_O, &err);
    if (stats != NULL) {
        if (n_ok > 0) {
            const char **rnames = gretl_matrix_get_rownames(stats);
            const char *label;

            for (t=0; t<stats->rows; t++) {
                label = rnames[t];
                fcdict[py::cast(label)] = stats->val[t];
            }
        }
        gretl_matrix_free(stats);
    }
}

//
// Native Single Equation libgretl MODEL (NSE)
// extends: GretlModel
//
GretlModel_NSE::GretlModel_NSE (std::shared_ptr<GretlDataset> GD)
                                : GretlModel(GD) // explicitly call base class constructor!
{}

GretlModel_NSE::~GretlModel_NSE (void)
{
    if (this->model != NULL) {
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
        handle_gretl_error(model->errcode);
    } else {
        printmodel(model, GD->dset, opt, GP.prn_ptr());
        GretlBundle b = GretlBundle(model, GD->dset);
        err = b.err;

        if (!err) {
            RES = b.to_dict();
            if (RES.empty()) {
                err = E_DATA;
                throw std::runtime_error("Model bundle to dict conversion failed.");
            } else {
                py::dict _ids;
                _ids[py::cast("_id")] = model->ID;
                _ids[py::cast("_model_id")] = id;
                RES[py::cast("_ids")] = _ids;
                RES[py::cast("summary")] = GP.get_buffer();
            }
        }
        if (err) {
            handle_gretl_error(err);
        } else {
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

py::dict GretlModel_NSE::forecast (const py::kwargs &kwargs)
{
    py::dict res;
    FITRESID *fr = NULL;
    FcastFlags flags = static_cast<FcastFlags>(0);
    int t1, t2 = 0;
#if 0 // not yet
    int recursive = 0, k = 1, *kptr;
#endif
    int dt2 = GD->dset->n - 1;
    int st2 = GD->dset->n - 1;
    double conf = 0.95;

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

    // assembling options (FIXME)
    unpack_kwargs_forecast(kwargs);
    fr = get_forecast(model, t1, t2, 0, GD->dset, OPT_NONE, &err);

    if (err) {
        handle_gretl_error(err);
    } else {
        fr_to_dict(fr, res);
        fr->alpha = 1 - conf;
        if (!(opt & OPT_Q)) {
            err = text_print_forecast(fr, GD->dset, OPT_NONE, GP.prn_ptr());
        }
    }

    if (fr != NULL) {
        free_fit_resid(fr);
    }

    GP.print();

    return res;
}

std::unique_ptr<GretlTypes> GretlModel_NSE::get_accessor (const std::string &accessor) const
{
    auto res = gretl_generate_gretl_object(accessor.c_str(), err, GRETL_OBJ_EQN, GD->dset, model);
    if (err) {
        handle_gretl_error(err);
    }

    return res;
}

py::dict GretlModel_NSE::test (const std::string &type,
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
            set_as_last_model(model, GRETL_OBJ_EQN);
        }
        err = model_test_driver(order, GD->dset, opt, GP.prn_ptr());
    } else {
        throw std::invalid_argument(type + " is a dataset-based test");
    }
    if (err) {
        handle_gretl_error(err);
    }

    res[py::cast("test")] = *get_accessor("$test");
    res[py::cast("pvalue")] = *get_accessor("$pvalue");
    res[py::cast("summary")] = GP.get_buffer();
    if (!kwargs.contains("silent")) {
        GP.print();
    }

    return res;
}

py::dict GretlModel_NSE::test (const std::string &type,
                               const std::string &vars,
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

    res[py::cast("test")] = *get_accessor("$test");
    res[py::cast("pvalue")] = *get_accessor("$pvalue");
    res[py::cast("summary")] = GP.get_buffer();
    if (!kwargs.contains("silent")) {
        GP.print();
    }

    return res;
}


py::dict GretlModel_NSE::restrict (const py::list &impose,
                                   const py::kwargs &kwargs)
{
    //
    // we test a restriction given in impose
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

    res[py::cast("test")] = *get_accessor("$test");
    res[py::cast("pvalue")] = *get_accessor("$pvalue");
    res[py::cast("summary")] = GP.get_buffer();
    if (!kwargs.contains("silent")) {
        GP.print();
    }

    return res;
}


//
// NSE via dedicated Driver
//
template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver<F, CI>::GretlModel_NSE_Driver (std::shared_ptr<GretlDataset> GD,
                                                     const std::string &formula,
                                                     const py::kwargs &kwargs)
                                                     : GretlModel_NSE(GD)  // explicitly call base class constructor!
{
    if constexpr (CI == TOBIT) {
        optional_par.resize(2);
        optional_par.at(0) = NADBL;
        optional_par.at(1) = NADBL;
    }
    if (!kwargs.empty()) {
        this->kwargs = kwargs;
    }
    this->formula = formula;
}

// Method template to support several estimation methods that
// have the same "basic" signature, plus a few special cases
// F: function to call
// CI: GretlCmdIndex

template <auto F, GretlCmdIndex CI>
void GretlModel_NSE_Driver<F, CI>::fit2 (void)
{
    fit(formula, kwargs);
}

template <auto F, GretlCmdIndex CI>
py::dict GretlModel_NSE_Driver<F, CI>::fit (const std::string &formula,
                                            const py::kwargs &kwargs)
{
    if (py::len(kwargs) > 0) {
        unpack_kwargs(kwargs, CI);
    }

    if (!GD) {
        throw std::runtime_error("No dataset is available");
    }

    // we construct the model
    var_list = list_from_formula(formula, GD->dset, err); // it's std::unique_ptr!!!
    model = gretl_model_new();

    if (var_list == nullptr) {
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
                                            const std::string &y,
                                            const py::tuple &tup,
                                            const py::kwargs &kwargs)
                                            : GretlModel_NSE(GD), y(y), tup(tup)  // explicitly call base class constructor!
{
    if (!kwargs.empty()) {
        this->kwargs = kwargs;
    }
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

void GretlModel_NSE_arima::fit2 (void)
{
    fit("", kwargs);
}

py::dict GretlModel_NSE_arima::fit (const std::string &formula,
                                    const py::kwargs &kwargs)
{
    int seasonal = 0;
    int y_number;
    int i;

    if (py::len(kwargs) > 0) {
        unpack_kwargs(kwargs, ARMA);
    }

    if (!GD) {
        throw std::runtime_error("No dataset is available");
    }

    y_number = current_series_index(GD->dset, y.c_str());
    if (y_number == -1) {
        throw std::invalid_argument("There is no variable: " + y);
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
    var_list.reset(gretl_list_new(seasonal ? 9 : 5)); // it's std::unique_ptr!!!
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

    *model = arma(var_list.get(), NULL, GD->dset, opt, GP.prn_ptr());
    save_model(opt);

    return RES;
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
                                        : GretlModel_NSE_Driver<F, CI>(GD, formula, kwargs)  // explicitly call base class constructor!
{
    this->optional_par.push_back(tau);
    if (!kwargs.empty()) {
        this->kwargs = kwargs;
    }
    this->formula = formula;
}

template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver_quantreg<F, CI>::GretlModel_NSE_Driver_quantreg
                                        (std::shared_ptr<GretlDataset> GD,
                                         const std::string &formula,
                                         const py::array_t<double> &tau,
                                         const py::kwargs &kwargs)
                                        : GretlModel_NSE_Driver<F, CI>(GD, formula, kwargs)  // explicitly call base class constructor!
{
    if (tau.ndim() != 1) {
        throw std::invalid_argument("'tau' is expected to be a vector");
    }
    this->optional_par_as_mat = gretl_matrix_alloc(tau.shape(0), 1);
    for (int i=0; i<tau.shape(0); i++) {
        gretl_matrix_set(this->optional_par_as_mat, i, 0, tau.at(i));
    }
    if (!kwargs.empty()) {
        this->kwargs = kwargs;
    }
    this->formula = formula;
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
