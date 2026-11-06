/*
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  gretl4py -- Python bindings for gretl
 *  Copyright (C) 2024 Marcin Błażejowski
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
#include "gretl4py_bundle.h"
#include "gretl4py_utils.h"
#include "gretl4py_packages.h"
#include "gretl_calls.h"
extern "C" {
    #include <gretl/var.h>
    #include <gretl/gretl_restrict.h>
    #include <gretl/varprint.h>
    #include <gretl/system.h>
    #include <gretl/compare.h>
}
#include <gretl/forecast.h>
#include <string>
#include <regex>
#include <algorithm>

#define GRETL4PY_MODEL_DEBUG 0

namespace {
template<GretlCmdIndex CI>
constexpr bool use_ldco_sig (void)
{
    //const int *list, DATASET *dset, GretlCmdIndex ci, gretlopt opt
    return CI == OLS || CI == WLS;
}

template<GretlCmdIndex CI>
constexpr bool use_ldcop_sig (void)
{
    //int *list, DATASET *dset, int ci, gretlopt opt, PRN *prn
    return CI == LOGIT || CI == PROBIT;
}

template<GretlCmdIndex CI>
constexpr bool use_ldo_sig (void)
{
    //const int *list, DATASET *dset, gretlopt opt
    return CI == LOGISTIC || CI == LAD || CI == MPOLS || CI == IVREG || CI == HSK;
}

template<GretlCmdIndex CI>
constexpr bool use_ldop_sig (void)
{
    //const int *list, DATASET *dset, gretlopt opt, PRN *prn
    return CI == BIPROBIT || CI == DURATION || CI == HECKIT || CI == AR || CI == AR1 || CI == GARCH || CI == PANEL || CI == INTREG;
}

template<GretlCmdIndex CI>
constexpr bool use_lcdop_sig (void)
{
    //const int *list, int ci, DATASET *dset, gretlopt opt, PRN *prn
    return CI == POISSON || CI == NEGBIN;
}

// Keep temporary plot series and a private dataset copy alive during plotting.
// The cleanup behavior matches the former TemporarySeriesGuard in plot().
class NSEPlotContext
{
private:
    MODEL *model_;                  // fitted single-equation model (not owned)
    GretlCmdIndex ci_;              // estimator command index
    const GretlDatasetPtr &source_; // original dataset (not owned)
    DATASET *dset_;                 // plotting dataset, possibly a private copy
    GretlPlot &plotter_;            // plot output owner
    PRN *prn_;                      // libgretl printer (not owned)
    int x_;                         // horizontal-axis variable number
    GretlDatasetPtr plot_data_;     // optional private dataset copy
    int origv_ = 0;                 // original variable count for attached data
    bool prepared_ = false;         // whether temporary data were prepared
    bool restore_ = false;          // whether to drop attached temporary series

    // Make a private copy only when using the user's dataset.
    // An attached model dataset can be modified temporarily.
    // Prepare the dataset and register temporary-series cleanup.
    void prepare_temporary_series_ (void)
    {
        if (prepared_) return;

        if (dset_ == source_->dset) {
            plot_data_ = GretlDataset::clone(*source_);
            dset_ = plot_data_->dset;
        } else {
            origv_ = dset_->v;
            restore_ = true;
        }
        prepared_ = true;
    }

    // Add fitted values or residuals with shared dataset cleanup.
    int add_model_series_ (ModelDataIndex which)
    {
        prepare_temporary_series_();
        return genr_fit_resid(model_, dset_, which);
    }

    // Register the temporary Gnuplot output for a libgretl command.
    void register_output_ (GretlCmdIndex command, const char *key, gretlopt opt)
    {
        gretl_valid_long_opt(opt, command, key, py::handle(), plotter_.filename().c_str());
    }
public:
    NSEPlotContext (MODEL *model, GretlCmdIndex ci, const GretlDatasetPtr &source,
                    DATASET *dset, GretlPlot &plotter, PRN *prn, int x)
    : model_(model), ci_(ci), source_(source), dset_(dset), plotter_(plotter), prn_(prn), x_(x)
    {}

    NSEPlotContext (const NSEPlotContext&) = delete;
    NSEPlotContext &operator= (const NSEPlotContext&) = delete;

    ~NSEPlotContext (void)
    {
        // Restore the attached model dataset after plotting.
        // A private dataset copy is cleaned up by its destructor.
        if (restore_ && dset_->v > origv_) {
            dataset_drop_last_variables(dset_, dset_->v - origv_);
        }
    }

    int fitted (void);

    int residuals (void);

    int correlogram (std::optional<int> order);

    int periodogram (bool bartlett, std::optional<int> width, bool log);

    int qqplot (void);
};

int NSEPlotContext::fitted (void)
{
    // based on fit_actual_plot() from gui/library.c
    gretlopt opt = OPT_A;
    int plotlist[4] = {0};

    // Force libgretl to write the plot to our temporary file.
    register_output_(GNUPLOT, "output", opt);

    // Prefer a fitted-value formula when available.
    gretl_char_ptr formula(gretl_model_get_fitted_formula(model_, x_, dset_));

    if (formula) {
        plotlist[0] = 3;
        plotlist[1] = 0;
        plotlist[2] = gretl_model_get_depvar(model_);
        plotlist[3] = x_;
        return gnuplot(plotlist, formula.get(), dset_, opt);
    }

    // Add fitted values temporarily.
    const int err = add_model_series_(M_YHAT);
    if (err) return err;

    plotlist[0] = 3;
    plotlist[1] = dset_->v - 1;
    plotlist[2] = gretl_model_get_depvar(model_);
    if (x_) {
        plotlist[3] = x_;
    } else {
        plotlist[0] = 2;
        SET_OPT(opt, OPT_T);
        if (dataset_is_time_series(dset_)) SET_OPT(opt, OPT_O);
    }
    return gnuplot(plotlist, nullptr, dset_, opt);
}

int NSEPlotContext::residuals (void)
{
    // Based on resid_plot() from gui/library.c.
    gretlopt opt = OPT_R;
    int plotlist[4] = {0};

    // Force libgretl to write the plot to our temporary file.
    register_output_(GNUPLOT, "output", opt);

    if (ci_ == GARCH && !(model_->opt & OPT_Z) && x_ == 0) {
        // Use libgretl's dedicated GARCH residual plot.
        return garch_resid_plot(model_, dset_);
    }

    // Add residuals temporarily.
    const int err = add_model_series_(M_UHAT);
    if (err) return err;

    const int uhatno = dset_->v - 1;
    const int yno = gretl_model_get_depvar(model_);

    plotlist[0] = 1;
    plotlist[1] = uhatno;
    strcpy(dset_->varname[uhatno], "residual");
    if (yno > 0) {
        std::string label = "residual for " + std::string(dset_->varname[yno]);

        series_set_label(dset_, uhatno, label.c_str());
    }
    if (x_) {
        // Plot residuals against the selected variable.
        plotlist[0] = 2;
        plotlist[2] = x_;
    } else {
        // Plot residuals against observations or time.
        SET_OPT(opt, OPT_T);
        if (dataset_is_time_series(dset_) || dataset_is_panel(dset_)) SET_OPT(opt, OPT_O);
    }

    // Match the GUI treatment of standardized GARCH residuals.
    if (ci_ == GARCH && (model_->opt & OPT_Z)) {
        series_set_display_name(dset_, uhatno, "standardized residual");
        opt = static_cast<gretlopt>(opt & ~OPT_R);
    }
    return gnuplot(plotlist, nullptr, dset_, opt);
}

int NSEPlotContext::correlogram (std::optional<int> order)
{
    // Plot the correlogram of model residuals.
    // Based on residual_correlogram_callback() from gui/library.c.

    // Add model residuals temporarily.
    const int err = add_model_series_(M_UHAT);
    if (err) return err;

    // Account for estimated ARMA coefficients in the Ljung-Box test.
    const int npq = model_->ci == ARMA ? arma_model_get_n_arma_coeffs(model_) : 0;

    // Use the GUI default lag order unless explicitly specified.
    const int lag_order = order.value_or(auto_acf_order(sample_size(dset_)));

    if (lag_order < 1) throw std::invalid_argument("order must be positive");

    // Register the correlogram plot output.
    register_output_(CORRGM, "plot", OPT_U);

    // Compute and plot the residual correlogram.
    const gretlopt opt = static_cast<gretlopt>(OPT_R | OPT_U);
    return corrgram(dset_->v - 1, lag_order, npq, dset_, opt, prn_);
}

int NSEPlotContext::periodogram (bool bartlett, std::optional<int> width, bool log)
{
    // Plot the periodogram of model residuals.
    // Based on residual_periodogram_callback() from gui/library.c.

    // Add model residuals temporarily.
    const int err = add_model_series_(M_UHAT);
    if (err) return err;

    // Use the GUI bandwidth default when Bartlett smoothing is requested.
    const int T = sample_size(dset_);
    const int bandwidth = bartlett ? width.value_or(auto_spectrum_order(T, OPT_O)) : 0;

    // The Bartlett window must fit within the estimation sample.
    if (bartlett && (bandwidth < 2 || bandwidth > T / 2)) throw py::value_error("width must be between 2 and half the sample size");

    // Request a plot file even in batch mode.
    register_output_(PERGM, "plot", OPT_U);
    gretlopt opt = OPT_U;
    if (bartlett) SET_OPT(opt, OPT_O);
    if (log) SET_OPT(opt, OPT_L);

    // Delegate spectral estimation and plotting to libgretl.
    return residual_periodogram(dset_->Z[dset_->v - 1], bandwidth, dset_, opt, prn_);
}

int NSEPlotContext::qqplot (void)
{
    // Plot the normal Q-Q graph of model residuals.
    // Based on residual_qq_plot() from gui/library.c.

    // Add model residuals temporarily.
    const int err = add_model_series_(M_UHAT);
    if (err) return err;

    int plotlist[4] = {0};
    plotlist[0] = 1;
    plotlist[1] = dset_->v - 1;

    // Route Q-Q plot output to the temporary Gnuplot file.
    register_output_(QQPLOT, "output", OPT_U);

    // Use the GUI defaults: sample-scaled normal quantiles and y = x.
    return qq_plot(plotlist, dset_, OPT_NONE);
}
} // namespace

//
// GretlModel class methods
//
GretlModel::GretlModel (GretlObjType model_obj_type, GretlCmdIndex ci, const py::kwargs &kwargs)
                        : id_(next_model_id_++), model_obj_type_(model_obj_type), ci_(ci), dataset_(GretlDataset::default_dataset())
{
    // we unpack kwargs
    if (py::len(kwargs) && ci_ != NC) {
        for (auto &item : kwargs) {
            if (!is_pystr(item.first)) throw std::invalid_argument("option key must be a string");
            const std::string &key = item.first.cast<std::string>();

            if (key == "data") {
                // special case: we use already allocated object of GretlDataset class
                this->dataset_ = item.second.cast<GretlDatasetPtr>();
                continue;
            } else if (key == "indepvars" && ci_ == ARMA) {
                // ARIMA special: here we only check if 'indepvars' is a string or list, but we must do something with it elsewhere
                if (!is_pystr(item.second) && !is_pylist(item.second)) throw std::invalid_argument("'indepvars' must be a string or list");
                continue;
            } else if (key == "seasonal" && ci_ == ARMA) {
                // ARIMA special: we check seasonal
                if (!is_pytuple(kwargs["seasonal"])) throw std::invalid_argument("'seasonal' must be a tuple");
                continue;
            } else if (key == "lagselect" && (ci_ == ARMA || ci_ == VAR)) {
                throw std::invalid_argument("'lagselect' option for (S)ARIMA and VAR is temporarily unavailable");
            } else if (key == "quiet" && !(ci_ == VAR || ci_ == VECM)) {
                throw std::invalid_argument("invalid option flag: " + key);
            } else if (key == "xlist" && (ci_ == VAR || ci_ == VECM)) {
                // VAR/VECM special: here we only check if xlist is a string or list, but we must do something with it elsewhere
                if (!is_pystr(item.second) && !is_pylist(item.second)) throw std::invalid_argument("'xlist' must be a string or list");
                continue;
            } else if (key == "rxlist" && ci_ == VECM) {
                // VECM special: here we only check if rxlist is a string or list, but we must do something with it elsewhere
                if (!is_pystr(item.second) && !is_pylist(item.second)) throw std::invalid_argument("'rxlist' must be a string or list");
                continue;
            } else if (key == "instruments" && ci_ == DPANEL) {
                // DPANEL special: here we only check if instruments is a string, but we must do something with it elsewhere
                if (!is_pystr(item.second)) throw std::invalid_argument("'instruments' must be a string");
                continue;
            } else if ((key == "llimit" || key == "rlimit") && ci_ == TOBIT) {
                // TOBIT special: we check (l/r)limit
                if (!is_pynumeric(item.second)) throw std::invalid_argument("both 'llimit' and 'rlimit' must be numeric");
                continue;
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
    if (!dataset_) handle_gretl_error(E_NODATA);
}

GretlModel::~GretlModel (void)
{
    if (this->dataset_) this->dataset_->linked_models_del(this->id_);
}

int GretlModel::get_id (void) const
{
    return id_;
}

//T = { MODEL, GRETL_VAR, equation_system }
template <typename T>
void GretlModel::save_model_ (T *model)
{
    //
    // Here we save estimation results, i.e. we set attributes on
    // a Python side using as intermediate step the $model or $system
    // bundle produced by libgretl.
    // We also add the @summary text produced by libgretl's printmodel().
    //
    // Check estimation errors before acquiring a libgretl reference.
    if constexpr (std::is_same_v<T, MODEL>) {
        err_ = model->errcode;
    } else if constexpr (std::is_same_v<T, GRETL_VAR>) {
        err_ = model->err;
    }
    if (err_) handle_gretl_error(err_);

    // Acquire the reference owned by this C++ wrapper.
    // This reference must be released in the model destructor.
    gretl_object_ref(model, model_obj_type_);
    model_registered_ = true;

    // we link model to the dataset used for estimation
    dataset_->linked_models_add(id_);

    // we print model
    if constexpr (std::is_same_v<T, MODEL>) {
        // we forse Pooled OLS heading if needed
        if (gretl_model_get_int(model, "pooled")) opt_ = static_cast<gretlopt>(opt_ & ~OPT_P);
        err_ = printmodel(model, dataset_->dset, opt_, printer_.prn_ptr());
    } else if constexpr (std::is_same_v<T, GRETL_VAR>) {
        err_ = gretl_VAR_print(model, dataset_->dset, opt_, printer_.prn_ptr());
    } else if constexpr (std::is_same_v<T, equation_system>) {
        // FIXME: needed?
        err_ = gretl_system_print(model, dataset_->dset, opt_, printer_.prn_ptr());
    } else {
        static_assert(always_false<T>, "GretlModel::save_model_(): unknown type");
    }
    if (err_) handle_gretl_error(err_);
    summary_ = std::string(printer_.get_buffer());   // for safety

    // we produce $model or $system bundle based on estimated @model
    if constexpr (std::is_same_v<T, equation_system>) {
        gb_model_ = std::make_shared<GretlBundle>(model);
    } else {
        gb_model_ = std::make_shared<GretlBundle>(model, dataset_->dset);
    }
    err_ = gb_model_->get_err();
    if (err_) handle_gretl_error(err_);

    // we finally register results on a python side
    py::object self = py::cast(this);
    gb_model_->python_register(self);

    // we record successful estimation
    estimated_ = true;
}

//T = { MODEL, GRETL_VAR, equation_system }
template <typename T>
GretlBundlePtr GretlModel::do_restrict_ (T *model, const py::list &impose, const py::kwargs &kwargs)
{
    //
    // we test a restriction given as impose
    // Note: this method is used in GretlModel_NSE
    // GretlModel_VAR_VECM, and GretlModel_NME
    //
    GretlBundlePtr res = std::make_shared<GretlBundle>();
    gretl_restriction *my_rset = nullptr;
    int save_t1 = dataset_->dset->t1;
    int save_t2 = dataset_->dset->t2;
    int order = -1;

    // we set up options
    unpack_kwargs_test(opt_, "restrict", kwargs, order);

    // this code was rendered from library.c
    for (auto &item : impose) {
        if (!is_pystr(item)) throw std::invalid_argument("given restriction is not string literal");
        auto item_str = item.cast<std::string>();

        gretl4py_trim(item_str);
        if (my_rset == nullptr) {
            if constexpr (std::is_same_v<T, MODEL>) {
                my_rset = eqn_restriction_set_start(item_str.c_str(), model, dataset_->dset, opt_);
            } else if constexpr (std::is_same_v<T, GRETL_VAR>) {
                my_rset = var_restriction_set_start(item_str.c_str(), model);
            } else if constexpr (std::is_same_v<T, equation_system>) {
                my_rset = cross_restriction_set_start(item_str.c_str(), model);
            }
            if (my_rset == nullptr) {
                err_ = 1;
                handle_gretl_error(err_);
            }
        } else {
            err_ = restriction_set_parse_line(my_rset, item_str.c_str(), dataset_->dset);
            if (err_) handle_gretl_error(err_);
        }
    }

    // actual work
    if constexpr (std::is_same_v<T, GRETL_VAR>) {
        GRETL_VAR *vnew = nullptr;

        vnew = gretl_restricted_vecm(my_rset, dataset_->dset, opt_, printer_.prn_ptr(), &err_);
        if (vnew != nullptr) gretl_VAR_free(vnew);
    } else {
        dataset_->dset->t1 = model->t1;
        dataset_->dset->t2 = model->t2;
        err_ = gretl_restriction_finalize(my_rset, dataset_->dset, OPT_NONE, printer_.prn_ptr());
    }
    if constexpr (std::is_same_v<T, equation_system>) {
        err_ = equation_system_estimate(model, dataset_->dset, OPT_NONE, printer_.prn_ptr());
    }
    dataset_->dset->t1 = save_t1;
    dataset_->dset->t2 = save_t2;
    if (err_) handle_gretl_error(err_);

    // we manufacture resulting object
    // material we return
    res->set_element("test", get_accessor("$test"));
    res->set_element("pvalue", get_accessor("$pvalue"));
    res->set_element<std::string>("summary", std::string(printer_.get_buffer()));

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, res->to_dict(), "restrictions");

    if (kwargs.contains("silent")) {
        auto val = kwargs["silent"];

        if (!is_bool_kwarg(val, "silent")) printer_.print();
    }

    return res;
}

//T = { MODEL, void }
template <typename T>
void GretlModel::gretl_do_forecast_ (void *model, const py::args &args, const py::kwargs &kwargs)
{
    //
    // method calls libgretl's do_forecast() function
    //
    static constexpr const int max_positional_args = 4;
    static constexpr std::array<const char*, max_positional_args> positional_args = {
        "startobs", "endobs", "steps_ahead", "vname"
    };
    GretlPlot plotter;
    std::string param = "";
    py::kwargs clean_kwargs = kwargs;
    int n_args = py::len(args);

    // lambda for param construction
    auto add_to_param = [&param](py::object &arg, const char *key = nullptr)
    {
        std::string s;

        if (is_pystr(arg)) {
            s = arg.cast<std::string>();
        } else if (is_pyint(arg)) {
            s = std::to_string(arg.cast<int>());
        } else if (is_pyfloat(arg)) {
            double x = arg.cast<double>();

            if (std::floor(x) != x) {
                std::string prefix = key && *key ? std::string(key) + "=" : "";

                handle_gretl_error(E_INVARG, "Invalid positional argument in forecast(): " + prefix + py::str(arg).cast<std::string>());
            }
            s = std::to_string(static_cast<long>(x));
        } else {
            handle_gretl_error(E_INVARG, "Invalid positional argument in forecast(): " + py::str(arg).cast<std::string>());
        }

        if (!param.empty()) param.append(" ");
        param += s;
    };

    // we unpack args
    if (n_args > max_positional_args) throw std::invalid_argument("too many positional arguments: " + std::to_string(py::len(args)) + " (max = 4)");
    if (n_args <= max_positional_args) {
        int left = max_positional_args - n_args;

        for (int i=0; i<n_args; ++i) {
            if (clean_kwargs.contains(positional_args[i])) throw py::type_error("multiple values for argument '" + std::string(positional_args[i]) + "'");
            py::object obj = args[i];

            add_to_param(obj);
        }
        if (!left) return;
        for (int i=max_positional_args-left; i<max_positional_args; ++i) {
            const char *key = positional_args[i];
            py::object obj = clean_kwargs.attr("pop")(key, py::none());

            if (!obj.is_none()) add_to_param(obj, key);
        }
    }

    // we work with kwargs
    opt_ = unpack_kwargs_command<FCAST>( dataset_, clean_kwargs, &plotter);

    // we do the job
    if constexpr (std::is_same_v<T, MODEL>) gretl_model_protect(static_cast<MODEL*>(model));
    set_as_last_model(model, model_obj_type_);
    err_ = do_forecast(param.c_str(), dataset_->dset, opt_, printer_.prn_ptr());
    if constexpr (std::is_same_v<T, MODEL>) gretl_model_unprotect(static_cast<MODEL*>(model));
    if (err_) handle_gretl_error(err_);
    printer_.print();
}

void GretlModel::check_if_estimated_ (void) const
{
    if (estimated_) throw std::runtime_error("model already estimated");
}

std::string GretlModel::repr_impl (void) const noexcept
{
    if (!estimated_) return "<Model: not fitted>";

    return summary_;
}

GretlBundlePtr GretlModel::get_bundle (void) const noexcept
{
    return gb_model_;
}

const GretlDatasetPtr GretlModel::get_dataset (void) const noexcept
{
    return dataset_;
}

[[nodiscard]] bool GretlModel::estimated (void) const noexcept
{
    return estimated_;
}

//T = { GRETL_VAR, equation_system }
template <typename T>
std::string GretlModel::do_save_uhat_ (T *model, int eqn, const std::string &name) const
{
    //
    // here take residuals from multivariate model
    // and add it to dataset
    // Note: eqn must be 1-based (not 0-based)
    //
    std::string uhat_name;
    gretl_double_ptr uhat{nullptr};

    // we retrieve uhat
    if constexpr (std::is_same_v<T, GRETL_VAR>) {
        uhat.reset(gretl_VAR_get_resid_series(model, eqn-1, &err_));
    } else if constexpr (std::is_same_v<T, equation_system>) {
        uhat.reset(system_get_resid_series(model, eqn-1, dataset_->dset, &err_));
    } else {
        static_assert(always_false<T>, "GretlModel::do_save_uhat_(): unknown type");
    }
    if (err_) handle_gretl_error(err_);

    // we set name
    if (name.empty()) uhat_name = std::string("m") + std::to_string(id_) + std::string("_uhat") + std::to_string(eqn);

    // actual work
    return dataset_->add_series(std::move(uhat), name.empty() ? uhat_name : name, dataset_->dset->n);
}

std::string GretlModel::extract_additional_vars_ (py::handle src) const
{
    //
    // here we produce gretl list with additional variables (xlist, zlist, etc)
    // passed withing kwargs as either string or Python's list
    //
    if (src.is_none()) return "";
    if (is_pystr(src)) return src.cast<std::string>();
    if (is_pylist(src)) return dataset_->python_list_to_formula(py::reinterpret_borrow<py::list>(src));
    throw std::invalid_argument("additional variables must be a string or a list");
}

void GretlModel::prepare_plot_(GretlPlot &plotter, gretlopt opt, bool multiple)
{
    //
    // convenient method for plotting pre-settings
    //
    // Multiple plots use SCATTERS; regular plots use GNUPLOT.
    const GretlCmdIndex ci = multiple ? SCATTERS : GNUPLOT;

    gretl_valid_long_opt(opt, ci, "output", py::handle(), plotter.filename().c_str());
}

//////////////////////////////////////////////////
// single equation models
//////////////////////////////////////////////////
//
// Native Single Equation libgretl MODEL (NSE)
// extends: GretlModel
//
GretlModel_NSE::GretlModel_NSE (GretlCmdIndex ci, const py::kwargs &kwargs) : GretlModel(GRETL_OBJ_EQN, ci, kwargs)
{
    model_ = gretl_model_new();
    if (!model_) handle_gretl_error(E_ALLOC);
}

GretlModel_NSE::~GretlModel_NSE (void)
{
    if (!model_) return;

    if (model_registered_) {
        gretl_object_unref(model_, model_obj_type_);
    } else {
        gretl_model_free(model_);
    }
}

template <GretlCmdIndex CI>
void GretlModel_NSE::regexp_extract_ (const std::string &formula, std::string &formula_left, std::string &formula_right)
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
    if (!formula_left.empty() && formula_left.back() == ';') formula_left.pop_back();
}

DATASET *GretlModel_NSE::get_model_dataset_ (gretlopt opt)
{
    //
    // this method mimics maybe_get_model_data() from gui/library.c
    //
    DATASET *dset = dataset_->dset;

    err_ = 0;
    if (gretl_is_between_model(model_)) {
        // Between models use their attached group-means dataset.
        dset = model_->dataset;
        if (!dset) {
            gretl_errmsg_set("The group-means dataset is not available");
            err_ = E_DATA;
        }
    } else if (model_sample_problem(model_, dset)) {
        // Reconstruct the estimation sample when subsamples differ.
        err_ = add_dataset_to_model(model_, dset, opt);
        if (!err_) dset = model_->dataset;
    }
    if (err_) handle_gretl_error(err_);

    return dset;
}

void GretlModel_NSE::forecast (const py::args &args, const py::kwargs &kwargs)
{
    gretl_do_forecast_<MODEL>(model_, args, kwargs);
}

py::object GretlModel_NSE::get_accessor (const std::string &name) const
{
    return do_get_accessor<GRETL_OBJ_EQN>(name, dataset_, static_cast<void*>(model_));
}

GretlBundlePtr GretlModel_NSE::test (const std::string &type, const py::kwargs &kwargs)
{
    //
    // this method performs test which do not take a variables list
    //
    GretlBundlePtr res = std::make_shared<GretlBundle>();
    int order = type == "corrgm" ? auto_acf_order(sample_size( dataset_->dset)) : dataset_->dset->pd;
    bool has_pvalue = true;     // if pvalue is accessible via $pvalue
    bool has_test = true;       // if test statistics is accessible via $test
    bool has_result = false;    // if given test produces additional results accessible via $result

    // we set up options
    auto printout = unpack_kwargs_test(opt_, type, kwargs, order);

    // we perform the test
    if (type == "bkw") {
        err_ = bkw_test(model_, dataset_->dset, opt_, printer_.prn_ptr());
        has_result = true;
    } else if (type == "cusum") {
        err_ = cusum_test(model_, dataset_->dset, opt_, printer_.prn_ptr());
    } else if (type == "leverage") {
        err_ = leverage_test(model_, dataset_->dset, opt_, printer_.prn_ptr());
        has_result = true;
    } else if (type == "panspec") {
        err_ = panel_specification_test(model_, dataset_->dset, opt_, printer_.prn_ptr());
    } else if (type == "reset") {
        err_ = reset_test(model_, dataset_->dset, opt_, printer_.prn_ptr());
    } else if (type == "qlrtest") {
        err_ = QLR_test(model_, dataset_->dset, opt_, printer_.prn_ptr());
    } else if (type == "vif") {
        err_ = vif_test(model_, dataset_->dset, opt_, printer_.prn_ptr());
        has_result = true;
        has_test = has_pvalue = false;
    } else if (type == "corrgm") {
        err_ = corrgram(dataset_->dset->v-1, order,
                        model_->ci == ARMA ? arma_model_get_n_arma_coeffs(model_) : 0, dataset_->dset, SET_OPT(opt_, OPT_R), printer_.prn_ptr());
    } else if (type == "arch" || type == "autocorr" || type == "breusch-pagan" || type == "comfac" ||
               type == "logs" || type == "normality" || type == "panel" || type == "squares" ||
               type == "white" || type == "white-nocross" || type == "xdepend" ) {
        if (get_last_model(nullptr) != model_) {
            err_ = gretl_model_protect(model_);
            if (err_) handle_gretl_error(err_);
            set_as_last_model(model_, model_obj_type_);
        }
        err_ = model_test_driver(order, dataset_->dset, opt_, printer_.prn_ptr());
        if (err_) handle_gretl_error(err_);
        err_ = gretl_model_unprotect(model_);
    } else {
        throw std::invalid_argument(type + " is a dataset-based test");
    }
    if (err_) handle_gretl_error(err_);

    // we print results if wanted
    if (printout) printer_.print();

    // we manufacture resulting object
    res->set_element<std::string>("type", type);
    res->set_element<std::string>("summary", std::string(printer_.get_buffer()));
    res->set_element("options", py::object(kwargs));
    if (has_test) res->set_element("test", get_accessor("$test"));
    if (has_pvalue) res->set_element("pvalue", get_accessor("$pvalue"));
    if (has_result) res->set_element("result", get_accessor("$result"));
    if (type == "qlrtest") res->set_element("qlrbreak", get_accessor("$qlrbreak"));

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, res->to_dict(), "tests");

    return res;
}

GretlBundlePtr GretlModel_NSE::test (const std::string &type, const py::object &varlist, const py::kwargs &kwargs)
{
    //
    // this method performs test which do take a variables list
    //
    GretlBundlePtr res = std::make_shared<GretlBundle>();
    gretl_list_ptr ex_var{nullptr};
    int order = -1;

    // we construct list of external variables
    if (type != "chow") {
        if (is_pystr(varlist)) {
            ex_var = dataset_->gretllist(varlist.cast<std::string>());
        } else if (is_pylist(varlist)) {
            ex_var = dataset_->gretllist(varlist.cast<py::list>());
        } else {
            throw std::invalid_argument("'varlist': unsupported type");
        }
    } else if (!is_pystr(varlist)) {
        throw std::invalid_argument("'varlist' must be a string");
    }

    // we set up options
    auto printout = unpack_kwargs_test(opt_, type, kwargs, order);

    // we perform the test
    if (type == "add") {
        err_ = add_test(model_, ex_var.get(), dataset_->dset, opt_, printer_.prn_ptr());
    } else if (type == "chow") {
        err_ = chow_test_driver(varlist.cast<std::string>().c_str(), model_, dataset_->dset, opt_, printer_.prn_ptr());
    } else if (type == "coeffsum") {
        err_ = gretl_sum_test(ex_var.get(), model_, dataset_->dset, opt_, printer_.prn_ptr());
    } else if (type == "omit") {
        err_ = omit_test(model_, ex_var.get(), dataset_->dset, opt_, printer_.prn_ptr());
    } else {
        throw std::invalid_argument(type + " is a dataset-based test");
    }
    if (err_) handle_gretl_error(err_);

    // we print results if wanted
    if (printout) printer_.print();

    // we manufacture resulting object
    res->set_element<std::string>("type", type);
    res->set_element("options", py::object(kwargs));
    res->set_element("test", get_accessor("$test"));
    res->set_element("pvalue", get_accessor("$pvalue"));
    res->set_element<std::string>("summary", std::string(printer_.get_buffer()));

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, res->to_dict(), "tests");

    return res;
}

template <GretlCmdIndex CI>
std::pair<GretlBundlePtr, GretlModelPtr> GretlModel_NSE::add_omit_auto (const py::kwargs &kwargs)
{
    //
    // this method performs ADD or OMIT tests with '--auto=' option
    //
    GretlBundlePtr res = std::make_shared<GretlBundle>();
    py::kwargs clean_kwargs = kwargs;
    gretl_list_ptr ex_var{nullptr};
    int order = -1;
    std::string type;

    if constexpr (CI == ADD) {
        py::object varlist = clean_kwargs.attr("pop")("varlist");

        if (is_pystr(varlist)) {
            ex_var = dataset_->gretllist(varlist.cast<std::string>());
        } else if (is_pylist(varlist)) {
            ex_var = dataset_->gretllist(varlist.cast<py::list>());
        } else {
            throw std::invalid_argument("'varlist': unsupported type");
        }
        type = "add";
    } else if constexpr (CI == OMIT) {
        type = "omit";
    } else {
        static_assert(always_false<std::integral_constant<GretlCmdIndex, CI>>, "GretlModel_NSE::add_omit_auto<CI>: unsupported CI");
    }

    // we set up options
    auto printout = unpack_kwargs_test(opt_, type, clean_kwargs, order);

    // we do the job
    auto m = std::make_unique<GretlModel_NSE_OLS_add_omit<CI>>(model_, dataset_, opt_, ex_var.get());
    m->fit();

    // we print results if wanted
    if (printout) printer_.print();

    // we manufacture resulting object
    res->set_element<std::string>("type", type);
    res->set_element("options", py::object(kwargs));
    res->set_element("test", get_accessor("$test"));
    res->set_element("pvalue", get_accessor("$pvalue"));
    res->set_element<std::string>("summary", std::string(printer_.get_buffer()));

    return {res, std::move(m)};
}
template std::pair<GretlBundlePtr, GretlModelPtr> GretlModel_NSE::add_omit_auto<ADD>(const py::kwargs&);
template std::pair<GretlBundlePtr, GretlModelPtr> GretlModel_NSE::add_omit_auto<OMIT>(const py::kwargs&);

GretlBundlePtr GretlModel_NSE::restrict (const py::list &impose, const py::kwargs &kwargs)
{
    return do_restrict_<MODEL>(model_, impose, kwargs);
}

template <ModelDataIndex IDX>
char *GretlModel_NSE::gretl_get_fit_or_resid (const std::string &name) const
{
    //
    // we save yhat, uhat, uhat^2, ahat, and h
    // Note: we return name of newly added series
    //
    char tmp_name[VNAMELEN] = {0};
    gchar *pdesc_raw = nullptr;
    gretl_double_ptr data(get_fit_or_resid(model_, dataset_->dset, IDX, tmp_name, &pdesc_raw, &err_));

    if (err_) handle_gretl_error(err_);

    // we save name and description
    std::string vname = name.empty() ? std::string(tmp_name) : name;
    gretl_gchar_ptr pdesc(pdesc_raw);

    return dataset_->add_series(std::move(data), vname, dataset_->dset->n, std::move(pdesc));
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

py::object GretlModel_NSE::plot (const std::string &type, const std::string &xvar, std::optional<int> order,
                                 bool bartlett, std::optional<int> width, bool log)
{
    //
    // general model plotter
    // For now: residuals or actual and fitted values against time or a selected variable.
    //
    if (!estimated_) throw std::runtime_error("model is not estimated");

    // Select the dataset corresponding to the estimation sample.
    GretlPlot plotter;
    DATASET *dset = get_model_dataset_(type == "correlogram" || type == "periodogram" || type == "qqplot" ? OPT_G : OPT_NONE);

    if (type == "correlogram" && !xvar.empty()) throw std::invalid_argument("correlogram does not accept xvar");
    if (type == "qqplot" && !xvar.empty()) throw py::type_error("qqplot does not accept xvar");
    if (type != "correlogram" && order.has_value()) throw std::invalid_argument("order is supported only for correlogram");
    if (type == "periodogram" && !xvar.empty()) throw py::type_error("periodogram does not accept xvar");
    // Periodogram settings must not silently affect other plot types.
    if (type != "periodogram" && (bartlett || width.has_value() || log))
        throw std::invalid_argument("bartlett, width and log are supported only for periodogram");
    if (type == "periodogram" && width.has_value() && !bartlett) throw std::invalid_argument("width requires bartlett=True");

    // Resolve the variable on the horizontal axis.
    int x = 0;

    if (!xvar.empty()) {
        x = current_series_index(dset, xvar.c_str());
        if (x < 0) throw std::invalid_argument("Unknown variable: " + xvar);
    }

    // Keep the selected dataset and any temporary series alive until display().
    NSEPlotContext ctx(model_, ci_, dataset_, dset, plotter, printer_.prn_ptr(), x);

    if (type == "fitted") {
        err_ = ctx.fitted();
    } else if (type == "residuals") {
        err_ = ctx.residuals();
    } else if (type == "correlogram") {
        err_ = ctx.correlogram(order);
    } else if (type == "periodogram") {
        err_ = ctx.periodogram(bartlett, width, log);
    } else if (type == "qqplot") {
        err_ = ctx.qqplot();
    } else {
        throw py::value_error("Unsupported plot type: " + type);
    }
    if (err_) handle_gretl_error(err_);
    if (type == "correlogram" || type == "periodogram") printer_.print();

    return plotter.display();
}

//
// NSE via arima plugin
//
GretlModel_NSE_arima::GretlModel_NSE_arima (const std::string &depvar, const py::tuple &tup, const py::kwargs &kwargs)
                                            : GretlModel_NSE(ARMA, kwargs), y_(depvar)
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
    y_number_ = this->dataset_->get_series_id(depvar, false);
    if (y_number_ == -1) handle_gretl_error(E_UNKVAR);

    // we set up non-seasonal parameters
    p_ = d_ = q_ = 0;
    get_arima_order(tup, p_, d_, q_);

    // we set up seasonal parameters
    if (kwargs.contains("seasonal") && !kwargs["seasonal"].is_none()) {
        const py::tuple seas = kwargs["seasonal"].cast<py::tuple>();

        P_ = D_ = Q_ = 0;
        get_arima_order(seas, P_, D_, Q_);
        if (P_ > 0 || D_ > 0 || Q_ > 0) seasonal_ = 1;
    }

    // we add exogenous variables if present
    if (kwargs.contains("indepvars")) {
        auto indepvars = extract_additional_vars_(kwargs["indepvars"]);

        if (!indepvars.empty()) {
            indepvars_ = this->dataset_->gretllist(indepvars);
            exogenous_ = indepvars_.get()[0];
        }
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
            var_list_.get()[i++] = indepvars_.get()[j];
        }
    }

    // do the job
    *model_ = arma(var_list_.get(), nullptr, dataset_->dset, opt_, printer_.prn_ptr());

    // save model
    save_model_<MODEL>(model_);
}

//
// NSE via dedicated Driver
//
template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver<F, CI>::GretlModel_NSE_Driver (const std::string &formula, const py::kwargs &kwargs)
                                                    : GretlModel_NSE(CI, kwargs), formula_(formula)
{
    // for DPANEL we manufacture var_list_ in class constructor
    if constexpr (CI == DPANEL) return;

    // we construct libgretl list of variables
    if constexpr (CI == MIDASREG) {
        std::string formula_left;

        formula_right_ = "";
        regexp_extract_<MIDASREG>(formula_, formula_left, formula_right_);
        var_list_ = dataset_->gretllist(formula_left);
    } else {
        var_list_ = dataset_->gretllist(formula_);
    }
}

template <auto F, GretlCmdIndex CI>
GretlModel_NSE_Driver<F, CI>::GretlModel_NSE_Driver (const py::list &varlist, const py::kwargs &kwargs)
                                                    : GretlModel_NSE(CI, kwargs)
{
    // for DPANEL we manufacture var_list_ in class constructor
    if constexpr (CI == DPANEL) return;

    // we construct libgretl list of variables
    var_list_ = dataset_->gretllist(varlist);
    var_list_from_list_ = true;
}

template <auto F, GretlCmdIndex CI>
std::string GretlModel_NSE_Driver<F, CI>::get_formula (void) const
{
    if (var_list_from_list_) {
        std::string formula_tmp;
        auto var_list_tmp = var_list_.get();

        for (int i=1; i<=var_list_tmp[0]; ++i) {
            if (var_list_tmp[i] == LISTSEP) {
                formula_tmp += ";";
            } else {
                formula_tmp += dataset_->dset->varname[var_list_tmp[i]];
            }
            formula_tmp += " ";
        }
        return gretl4py_trim(formula_tmp);
    }

    return formula_;
}

// Method template to support several estimation methods that
// have the same "basic" signature, plus a few special cases
// F: function to call
// CI: GretlCmdIndex
template <auto F, GretlCmdIndex CI>
void GretlModel_NSE_Driver<F, CI>::fit (void)
{
    // we check if we deal with clean model
    check_if_estimated_();

    // we call F as an estimator
    if constexpr (use_ldco_sig<CI>()) {
        // fallback to Pooled OLS if needed
        if (CI == OLS && dataset_->is_panel()) {
            SET_OPT(opt_, (OPT_P | OPT_L));
            *model_ = panel_model(var_list_.get(), dataset_->dset, opt_, printer_.prn_ptr());
        } else {
            *model_ = F(var_list_.get(), dataset_->dset, CI, opt_);
        }
    } else if constexpr (use_ldcop_sig<CI>()) {
        *model_ = F(var_list_.get(), dataset_->dset, static_cast<int>(CI), opt_, printer_.prn_ptr());
    } else if constexpr (use_ldo_sig<CI>()) {
        *model_ = F(var_list_.get(), dataset_->dset, opt_);
    } else if constexpr (use_ldop_sig<CI>()) {
        *model_ = F(var_list_.get(), dataset_->dset, opt_, printer_.prn_ptr());
    } else if constexpr (use_lcdop_sig<CI>()) {
        *model_ = F(var_list_.get(), static_cast<int>(CI), dataset_->dset, opt_, printer_.prn_ptr());
    } else if constexpr (CI == MIDASREG) {
        *model_ = F(var_list_.get(), formula_right_.c_str(), dataset_->dset, opt_, printer_.prn_ptr());
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
GretlModel_NSE_Driver_quantreg::GretlModel_NSE_Driver_quantreg (const std::string &formula, const double &tau, const py::kwargs &kwargs)
                                                                : GretlModel_NSE_Driver<quantreg_driver, QUANTREG>(formula, kwargs), tau_(std::to_string(tau))
{}

GretlModel_NSE_Driver_quantreg::GretlModel_NSE_Driver_quantreg (const std::string &formula, const py::array_t<double> &tau, const py::kwargs &kwargs)
                                                                : GretlModel_NSE_Driver<quantreg_driver, QUANTREG>(formula, kwargs)
{
    if (tau.ndim() != 1) throw std::invalid_argument("'tau' is expected to be a vector");
    tau_ = "{";
    for (int i=0; i<tau.shape(0); ++i) {
        tau_.append(std::to_string(tau.at(i)));
        if (i < tau.shape(0)-1) tau_.append(";");
    }
    tau_.append("}");
}

GretlModel_NSE_Driver_quantreg::GretlModel_NSE_Driver_quantreg (const py::list &varlist, const double &tau, const py::kwargs &kwargs)
                                                                : GretlModel_NSE_Driver<quantreg_driver, QUANTREG>(varlist, kwargs), tau_(std::to_string(tau))
{}

GretlModel_NSE_Driver_quantreg::GretlModel_NSE_Driver_quantreg (const py::list &varlist, const py::array_t<double> &tau, const py::kwargs &kwargs)
                                                                : GretlModel_NSE_Driver<quantreg_driver, QUANTREG>(varlist, kwargs)
{
    if (tau.ndim() != 1) throw std::invalid_argument("'tau' is expected to be a vector");
    tau_ = "{";
    for (int i=0; i<tau.shape(0); ++i) {
        tau_.append(std::to_string(tau.at(i)));
        if (i < tau.shape(0)-1) tau_.append(";");
    }
    tau_.append("}");
}

void GretlModel_NSE_Driver_quantreg::fit (void)
{
    // we check if we deal with clean model
    check_if_estimated_();

    // we do the job
    *model_ = quantreg_driver(tau_.c_str(), var_list_.get(), dataset_->dset, opt_, printer_.prn_ptr());

    // we save results
    save_model_<MODEL>(model_);
}

//
// NSE_Driver_dpanel via dpd_model
// extends: GretlModel_NSE_Driver
//
GretlModel_NSE_Driver_dpanel::GretlModel_NSE_Driver_dpanel (const std::string &formula, const int &p, const py::kwargs &kwargs)
                                                            : GretlModel_NSE_Driver<dpd_model, DPANEL>(formula, kwargs)
{
    lags_.push_back(p);

    // we construct libgretl list of variables
    std::string formula_left;

    formula_right_ = "";
    regexp_extract_<DPANEL>(formula_, formula_left, formula_right_);
    var_list_ = dataset_->gretllist(formula_left);
}

GretlModel_NSE_Driver_dpanel::GretlModel_NSE_Driver_dpanel (const std::string &formula, const py::array_t<int> &p, const py::kwargs &kwargs)
                                                            : GretlModel_NSE_Driver<dpd_model, DPANEL>(formula, kwargs)
{
    if (p.ndim() != 1) throw std::invalid_argument("'p' is expected to be a vector");
    for (int i=0; i<p.shape(0); ++i) {
        lags_.push_back(static_cast<double>(p.at(i)));
    }

    // we construct libgretl list of variables
    std::string formula_left;

    formula_right_ = "";
    regexp_extract_<DPANEL>(formula_, formula_left, formula_right_);
    var_list_ = dataset_->gretllist(formula_left);
}

void GretlModel_NSE_Driver_dpanel::fit (void)
{
    gretl_list_ptr dpanel_list{nullptr};
    gretl_list_ptr laglist{nullptr};

    // we check if we deal with clean model
    check_if_estimated_();

    // we setup dpanel_list: lags + var_list
    dpanel_list.reset(gretl_list_new(var_list_.get()[0] + 2));
    if (!dpanel_list) handle_gretl_error(E_ALLOC);
    if (lags_.size() == 1) {
        dpanel_list.get()[1] = static_cast<int>(lags_.at(0));
    } else {
        dpanel_list.get()[1] = static_cast<int>(*std::max_element(lags_.begin(), lags_.end()));
    }
    dpanel_list.get()[2] = LISTSEP;
    std::memcpy(dpanel_list.get() + 3, var_list_.get() + 1, var_list_.get()[0] * sizeof(int));

    // we do the job
    if (lags_.size() > 1) {
        laglist.reset(gretl_list_new(lags_.size()));
        if (!laglist) handle_gretl_error(E_ALLOC);
        for (size_t i=0; i<lags_.size(); i++) {
            laglist.get()[i+1] = static_cast<int>(lags_[i]);
        }
    }
    *model_ = dpd_model(dpanel_list.get(), laglist.get(), formula_right_.c_str(), dataset_->dset, opt_, printer_.prn_ptr());

    // we save results
    save_model_<MODEL>(model_);
}

//
// NSE_Driver_tobit via tobit_model
// extends: GretlModel_NSE_Driver
//
GretlModel_NSE_Driver_tobit::GretlModel_NSE_Driver_tobit (const std::string &formula, const py::kwargs &kwargs)
                                                            : GretlModel_NSE_Driver<tobit_model, TOBIT>(formula, kwargs)
{
    // we set (l/r)limit
    if (kwargs.contains("llimit") && !kwargs["llimit"].is_none()) llimit_ = py::cast<double>(kwargs["llimit"]);
    if (kwargs.contains("rlimit") && !kwargs["rlimit"].is_none()) rlimit_ = py::cast<double>(kwargs["rlimit"]);
}

GretlModel_NSE_Driver_tobit::GretlModel_NSE_Driver_tobit (const py::list &varlist, const py::kwargs &kwargs)
                                                            : GretlModel_NSE_Driver<tobit_model, TOBIT>(varlist, kwargs)
{
    // we set (l/r)limit
    if (kwargs.contains("llimit") && !kwargs["llimit"].is_none()) llimit_ = py::cast<double>(kwargs["llimit"]);
    if (kwargs.contains("rlimit") && !kwargs["rlimit"].is_none()) rlimit_ = py::cast<double>(kwargs["rlimit"]);
}

void GretlModel_NSE_Driver_tobit::fit (void)
{
    // we check if we deal with clean model
    check_if_estimated_();

    // we do the job
    *model_ = tobit_model(var_list_.get(), llimit_, rlimit_, dataset_->dset, opt_, printer_.prn_ptr());

    // we save results
    save_model_<MODEL>(model_);
}

//
// OLS for add/omit tests
// extends: GretlModel_NSE_Driver<lsq, OLS>
//
template <GretlCmdIndex CI>
GretlModel_NSE_OLS_add_omit<CI>::GretlModel_NSE_OLS_add_omit (MODEL *src, GretlDatasetPtr src_dataset, gretlopt opt, const int *addvars)
                                    : GretlModel_NSE_Driver<lsq, OLS>(), addvars_(addvars)
{
    if (!src) throw std::runtime_error("didn't got model");
    if (!src_dataset) handle_gretl_error(E_NODATA);
    src_model_ = src;
    dataset_ = src_dataset;
    test_opt_ = opt;
    gretl_model_init(model_, dataset_->dset);
    if (!model_) handle_gretl_error(E_ALLOC);
}

template <GretlCmdIndex CI>
GretlModel_NSE_OLS_add_omit<CI>::~GretlModel_NSE_OLS_add_omit (void) = default;

template <GretlCmdIndex CI>
void GretlModel_NSE_OLS_add_omit<CI>::fit (void)
{
    // we check if we deal with clean model
    check_if_estimated_();

    // we do the job
    if constexpr (CI == ADD) {
        err_ = add_test_full(src_model_, model_, addvars_, dataset_->dset, test_opt_, printer_.prn_ptr());
    } else if constexpr (CI == OMIT) {
        err_ = omit_test_full(src_model_, model_, nullptr, dataset_->dset, test_opt_, printer_.prn_ptr());
    } else {
        static_assert(always_false<std::integral_constant<GretlCmdIndex, CI>>, "GretlModel_NSE::add_omit_auto<CI>: unsupported CI");
    }

    printer_.print();

    // if there was no change in a varlist, we don't save model
    if (err_ != E_NOADD && err_ != E_NOOMIT) {
        if (err_) handle_gretl_error(err_);
        save_model_<MODEL>(model_);
    }
}

// explicit instantiations for ADD and OMIT
template class GretlModel_NSE_OLS_add_omit<ADD>;
template class GretlModel_NSE_OLS_add_omit<OMIT>;

//
// NLS,MLE,GMM via plugin
// extends: GretlModel_NSE
//
template <GretlCmdIndex CI>
GretlModel_NSE_nl<CI>::GretlModel_NSE_nl (const std::string formula, const py::kwargs &kwargs)
                                            : GretlModel_NSE(CI, kwargs), command_(gretl_command_word(CI)), formula_(formula)
{
    // we get model definition
    get_object_from_dict<std::string>(kwargs, "params", params_);
    get_object_from_dict<std::string>(kwargs, "param_names", param_names_);
    if (!param_names_.empty()) param_names_ = "\"" + param_names_ + "\"";

    // we set up initial values
    if (kwargs.contains("initvals")) {
        if (!is_pydict(kwargs["initvals"])) throw std::invalid_argument("'initvals' must be a dictionary");
        py::dict initvals = kwargs["initvals"].cast<py::dict>();

        for (const auto &[key, val] : initvals) {
            gretl_generate<GRETL_TYPE_ANY>(py::str(val), dataset_, py::cast<std::string>(key));
        }
    }

    // special cases
    if constexpr (CI == NLS || CI == MLE) {
        // NLS and MLE special case: we get derivatives
        if (kwargs.contains("deriv")) {
            auto deriv = kwargs["deriv"];

            if (is_pydict(deriv)) {
                for (const auto &[key, val] : deriv.cast<py::dict>()) {
                    deriv_.push_back(py::cast<std::string>(key) + " = " + py::str(val).cast<std::string>());
                }
            } else if (is_pylist(deriv)) {
                deriv_ = py::cast<std::vector<std::string>>(deriv);
            } else {
                throw std::invalid_argument("'deriv' must be either a list or dict");
            }
        }
    } else if constexpr (CI == GMM) {
        get_object_from_dict<std::vector<std::string>>(kwargs, "orthog", orthog_);
        get_object_from_dict<std::string>(kwargs, "weights", weights_);
    }

    // we get statements entries
    if constexpr (CI == MLE || CI == GMM) get_object_from_dict<std::vector<std::string>>(kwargs, "statements", statements_);
}

template <GretlCmdIndex CI>
GretlModel_NSE_nl<CI>::~GretlModel_NSE_nl (void) = default;

template <GretlCmdIndex CI>
template <typename T>
void GretlModel_NSE_nl<CI>::do_nl_parse_line_ (const std::string &keyword, T *src)
{
    //
    // here we perform line parsing and adding (to libgretl)
    // Note: to add formula, use "command" as keyword and nullptr as src
    //
    // argument validation:
    // 1. keyword must not be empty
    // 2. if keyword != "command", src must not be null
    if (keyword.empty() || (keyword != "command" && !src)) throw std::invalid_argument("do_nl_parse_line_(): wrong arguments passed");

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
        err_ = nl_parse_line(CI, line.c_str(), dataset_->dset, printer_.prn_ptr());
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
            err_ = nl_parse_line(CI, line.c_str(), dataset_->dset, printer_.prn_ptr());
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
    *model_ = nl_model( dataset_->dset, opt_, printer_.prn_ptr());

    // we save results
    save_model_<MODEL>(model_);
}

template <GretlCmdIndex CI>
std::string GretlModel_NSE_nl<CI>::get_formula (void) const
{
    std::string res = formula_ + "\n";

    if constexpr (CI == NLS || CI == MLE) {
        if (deriv_.size()) {
            for (const auto &el : deriv_) {
                res += "deriv " + el + "\n";
            }
        }
    }
    if constexpr (CI == GMM) {
        if (orthog_.size()) {
            for (const auto &el : orthog_) {
                res += "orthog " + el + "\n";
            }
        }

        if (!weights_.empty()) {
            res += "weights " + weights_ + "\n";
        }
    }
    if constexpr (CI == MLE || CI == GMM) {
        if (statements_.size()) {
            for (const auto &el : statements_) {
                res += el + "\n";
            }
        }
    }
    if (!params_.empty()) res += "params " + params_ + "\n";
    if (!param_names_.empty()) res += "param_names " + param_names_ + "\n";

    return res;
}

template <GretlCmdIndex CI>
template <bool ForString>
void GretlModel_NSE_nl<CI>::add_line_to_parse (const std::string &keyword, const std::string &val)
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
void GretlModel_NSE_nl<CI>::add_init_value (const std::string &varname, double value)
{
    //
    // adds/sets value of varname in the contexts of GD
    //
    gretl_generate<GRETL_TYPE_ANY>(std::to_string(value), dataset_, varname);
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
GretlModel_VAR::GretlModel_VAR (const std::string &ylist, const int order, GretlCmdIndex ci, const py::kwargs &kwargs)
                                : GretlModel(GRETL_OBJ_VAR, ci, kwargs), vars_(ylist), order_(order)
{
    if (kwargs.contains("xlist")) {
        auto xlist = extract_additional_vars_(kwargs["xlist"]);

        if (!xlist.empty()) vars_.append(" ; ").append(xlist);
    }
    if (ci == VECM) return;

    // we construct libgretl list of variables
    var_list_ = dataset_->gretllist(vars_);
}

GretlModel_VAR::GretlModel_VAR (const py::list &varlist, const int order, GretlCmdIndex ci, const py::kwargs &kwargs)
                                : GretlModel(GRETL_OBJ_VAR, ci, kwargs), order_(order)
{
    std::string xlist_str;

    // for VECM the base constructor only extracts the optional xlist,
    // so the derived constructor appends rxlist (if any) and builds vars_
    if (kwargs.contains("xlist")) xlist_str = extract_additional_vars_(kwargs["xlist"]);
    if (ci == VECM) {
        vars_ = xlist_str;
        return;
    }

    // we build the libgretl list of endogenous variables
    auto varlist_ptr = dataset_->gretllist(varlist);
    gretl_list_ptr xlist{nullptr};

    // we build vars_
    vars_ = gretl_list_to_string(varlist_ptr.get(), dataset_->dset, &err_);
    if (err_) handle_gretl_error(err_);
    gretl4py_trim(vars_);

    // if xlist is supplied, convert it to a libgretl list and join it to var_list_
    if (!xlist_str.empty()) {
        xlist = dataset_->gretllist(xlist_str);
        if (!xlist) handle_gretl_error(E_UNKVAR);
        vars_ += " ; " + xlist_str;
    }
    if (xlist) {
        auto joined = gretl_lists_join_with_separator(varlist_ptr.get(), xlist.get());

        if (!joined) handle_gretl_error(E_ALLOC);
        var_list_.reset(joined);
    } else {
        var_list_ = std::move(varlist_ptr);
    }
}

GretlModel_VAR::~GretlModel_VAR (void)
{
    gretl_object_unref(this->model_, this->model_obj_type_);
}

void GretlModel_VAR::fit (void)
{
    // we check if we deal with clean model
    check_if_estimated_();

    // we do the job
    model_ = gretl_VAR(order_, nullptr, var_list_.get(), dataset_->dset, opt_, printer_.prn_ptr(), &err_);
    if (err_) handle_gretl_error(err_);

    // save results
    save_model_<GRETL_VAR>(model_);
}

void GretlModel_VAR::forecast (const py::args &args, const py::kwargs &kwargs)
{
    gretl_do_forecast_<void>(model_, args, kwargs);
}

GretlBundlePtr GretlModel_VAR::test (const std::string &type, const py::kwargs &kwargs)
{
    GretlBundlePtr res = std::make_shared<GretlBundle>();
    int order = dataset_->dset->pd;

    // basic check
    if (type == "normality" && kwargs.contains("order")) throw std::invalid_argument("normality test doesn't take 'order' option");

    // we set up options
    unpack_kwargs_test(opt_, type, kwargs, order);

    // we perform test
    if (type == "autocorr") {
        err_ = gretl_VAR_autocorrelation_test(model_, order, dataset_->dset, opt_, printer_.prn_ptr());
    } else if (type == "arch") {
        err_ = gretl_VAR_arch_test(model_, order, dataset_->dset, opt_, printer_.prn_ptr());
    } else if (type == "normality") {
        err_ = gretl_VAR_normality_test(model_, opt_, printer_.prn_ptr());
    } else {
        throw std::invalid_argument(type + ": unsupported test");
    }
    if (err_) handle_gretl_error(err_);

     // we manufacture resulting object
    res->set_element<std::string>("type", type);
    res->set_element("options", py::object(kwargs));
    res->set_element("test", get_accessor("$test"));
    res->set_element("pvalue", get_accessor("$pvalue"));
    res->set_element<std::string>("summary", std::string(printer_.get_buffer()));

    // we register result
    move_to_python_container<GretlModel, py::dict, py::list>(*this, res->to_dict(), "tests");

    if (kwargs.contains("silent")) {
        auto val = kwargs["silent"];

        if (!is_bool_kwarg(val, "silent")) printer_.print();
    }

    return res;
}

std::string GretlModel_VAR::get_formula (void) const
{
    return vars_;
}

py::object GretlModel_VAR::get_accessor (const std::string &name) const
{
    return do_get_accessor<GRETL_OBJ_VAR>(name, dataset_, static_cast<void*>(model_));
}

std::string GretlModel_VAR::save_uhat (int eqn, const std::string &name) const
{
    // guard for cases when in previous call we wanted uhat from non-existing equation
    if (err_ == E_BADSTAT) err_ = 0;

    return do_save_uhat_<GRETL_VAR>(model_, eqn, name);
}

int GretlModel_VAR::find_variable_nr_ (const std::string &variable_name)
{
    //
    // Find the zero-based equation index of an endogenous variable
    // in the fitted VAR/VECM model.
    //
    for (int i=1; i<=model_->neqns; ++i) {
        if (variable_name == dataset_->get_series_name(model_->ylist[i])) return i - 1;
    }

    return -1; // not found
}

int GretlModel_VAR::find_variable_nr_ (int id)
{
    //
    // Find the zero-based equation index of an endogenous variable
    // using its dataset variable ID.
    //
    for (int i=1; i<=model_->neqns; ++i) {
        if (model_->ylist[i] == id) return i - 1;
    }

    return -1; // not found
}

GretlMatrixPtr GretlModel_VAR::irf (int target, int shock, double alpha)
{
    //
    // this method mimics gretl's irf() function
    //
    // we compute IRFs
    gretl_matrix_ptr res_irf(gretl_VAR_get_impulse_response(model_, target, shock, default_VAR_horizon(dataset_->dset), alpha, dataset_->dset, &err_));

    if (err_) handle_gretl_error(err_);

    return std::make_shared<GretlMatrix>(res_irf.release(), false);
}

GretlMatrixPtr GretlModel_VAR::irf (const std::string &target, const std::string &shock, double alpha)
{
    //
    // this method mimics gretl's irf() function
    // Note: overload which takes names instead of equation number
    //
    int target_int, shock_int;

    // let's try to find our target/shock IDs
    try {
        target_int = std::stoi(target);
    } catch (std::invalid_argument &e) {
        target_int = find_variable_nr_(target);
    }
    if (target_int == -1) throw std::invalid_argument("target value is invalid");
    try {
        shock_int = std::stoi(shock);
    } catch (std::invalid_argument &e) {
        shock_int = find_variable_nr_(shock);
    }
    if (shock_int == -1) throw std::invalid_argument("shock value is invalid");

    // we do the job
    return irf(target_int, shock_int, alpha);
}

template <typename T>
void GretlModel_VAR::set_cholesky_order (const T &varlist)
{
    //
    // Set the Cholesky ordering of endogenous variables.
    // An empty list restores the default VAR/VECM equation order.
    //
    if (!estimated_) throw std::runtime_error("VAR/VECM model has not been estimated");

    gretl_list_ptr vars{nullptr};
    err_ = 0;

    // Convert the variable specification to dataset variable IDs.
    if (varlist.empty()) {
        err_ = gretl_VAR_set_ordering(model_, nullptr);
        if (err_) handle_gretl_error(err_);
        return;
    } else {
        if constexpr (is_gretllist_arg_v<T>) {
            vars = dataset_->gretllist(varlist);
        } else {
            static_assert(always_false<T>, "GretlModel_VAR::set_cholesky_order(): unsupported type");
        }
    }
    if (vars.get()[0] != model_->neqns) throw std::invalid_argument("Cholesky ordering must include all endogenous variables");

    // Convert dataset IDs to zero-based VAR equation indices.
    // Check that the ordering is a valid permutation.
    std::vector<int> indices;
    std::vector<bool> seen(model_->neqns, false);
    bool default_order = true;

    indices.reserve(model_->neqns);

    for (int i=1; i<=model_->neqns; ++i) {
        const int idx = find_variable_nr_(vars.get()[i]);

        if (idx < 0) throw std::invalid_argument("Cholesky ordering contains a non-endogenous variable");
        if (seen[idx]) throw std::invalid_argument("Cholesky ordering contains duplicate variables");

        seen[idx] = true;
        indices.push_back(idx);

        if (idx != i - 1) default_order = false;
    }

    // Restore the default ordering, if requested.
    if (default_order) {
        err_ = gretl_VAR_set_ordering(model_, nullptr);
        if (err_) handle_gretl_error(err_);
        return;
    }

    // Construct the ordering vector and transfer its ownership to libgretl.
    gretl_matrix_ptr ordvec(gretl_vector_alloc(model_->neqns));

    if (!ordvec) handle_gretl_error(E_ALLOC);
    for (int i=0; i<model_->neqns; ++i) {
        ordvec->val[i] = indices[i];
    }

    err_ = gretl_VAR_set_ordering(model_, ordvec.release());
    if (err_) handle_gretl_error(err_);
}
template void GretlModel_VAR::set_cholesky_order<std::string>(const std::string&);
template void GretlModel_VAR::set_cholesky_order<py::list>(const py::list&);

py::object GretlModel_VAR::plot (const std::string &type, std::optional<int> equation, std::optional<std::string> mode,
                                 std::optional<int> periods, std::optional<double> alpha, std::optional<bool> error_bars,
                                 std::variant<int, std::string> targ, std::variant<int, std::string> shock,
                                 std::optional<bool> histogram)
{
    //
    // general VAR plotter, i.e. we deal with residuals, roots and IRFs
    //
    GretlPlot plotter;
    const int eqn = equation.value_or(0);
    const std::string plot_mode = mode.value_or("combined");

    auto resolve = [this](const std::variant<int, std::string> &arg, const char *name) -> int
    {
        //
        // Resolve target and shock to zero-based equation numbers.
        // String arguments identify endogenous variables by name.
        //
        int idx;

        if (auto p = std::get_if<int>(&arg)) {
            idx = *p;
        } else {
            idx = find_variable_nr_(std::get<std::string>(arg));
        }

        if (idx < 0 || idx >= model_->neqns) {
            throw py::value_error(std::string("Invalid VAR target/shock '") + name
            + "': expected an endogenous variable name or a zero-based "
            "equation index in [0, " + std::to_string(model_->neqns - 1) + "]");
        }

        return idx;
    };

    // At first, we reject bad options
    if (!estimated_) throw std::runtime_error("model is not estimated");
    if (type != "residuals" && type != "inverse_roots" && type != "irf"
        && type != "irf_combined" && type != "fevd") throw std::invalid_argument("Unsupported plot type: " + type);

    if (type == "inverse_roots") {
        if (equation.has_value() || mode.has_value()) throw py::type_error("inverse_roots does not accept equation or mode");
    } else {
        if (plot_mode != "combined" && plot_mode != "multiple") throw std::invalid_argument("Unsupported residual plot mode: " + plot_mode);
        if (eqn < 0 || eqn > model_->neqns) throw std::invalid_argument("Equation number is out of range");
        if (plot_mode == "multiple" && eqn != 0) throw std::invalid_argument("Equation must be 0 for multiple residual plots");
    }

    // Register the output file for the selected plot type.
    prepare_plot_(plotter, OPT_NONE, plot_mode == "multiple");

    // we do the job
    if (type == "inverse_roots") {
        err_ = gretl_VAR_roots_plot(model_);
    } else if (type == "irf_combined") {
        err_ = gretl_VAR_plot_multiple_irf(model_, periods.value_or(default_VAR_horizon(dataset_->dset)),
                                           alpha.value_or(0.0), dataset_->dset, error_bars.value_or(false));
    } else if (type == "irf") {
        err_ = gretl_VAR_plot_impulse_response(model_, resolve(targ, "targ"), resolve(shock, "shock"),
                                               periods.value_or(default_VAR_horizon(dataset_->dset)),
                                               alpha.value_or(0.0), dataset_->dset, error_bars.value_or(false));
    } else if (type == "fevd") {
        err_ = gretl_VAR_plot_FEVD(model_, resolve(targ, "targ"), periods.value_or(default_VAR_horizon(dataset_->dset)),
                                   dataset_->dset, histogram.value_or(true));
    } else if (plot_mode == "multiple") {
        err_ = gretl_system_residual_mplot(model_, ci_, dataset_->dset);
    } else {
        err_ = gretl_system_residual_plot(model_, ci_, eqn, dataset_->dset);
    }
    if (err_) handle_gretl_error(err_);

    return plotter.display();
}

//
// VECM libgretl MODEL
// extends: GretlModel_VAR
//
GretlModel_VAR_VECM::GretlModel_VAR_VECM (const std::string &ylist, const int order, const int rank, const py::kwargs &kwargs)
                                            : GretlModel_VAR(ylist, order, VECM, kwargs), coint_rank_(rank)
{
    if (kwargs.contains("rxlist")) {
        auto rxlist = extract_additional_vars_(kwargs["rxlist"]);

        if (!rxlist.empty()) vars_.append(" ; ").append(rxlist);
    }

    // we construct libgretl list of variables
    var_list_ = dataset_->gretllist(vars_);
}

GretlModel_VAR_VECM::GretlModel_VAR_VECM (const py::list &varlist, const int order, const int rank, const py::kwargs &kwargs)
                                            : GretlModel_VAR(varlist, order, VECM, kwargs), coint_rank_(rank)
{
    // we extract the optional rxlist
    // note: at this point vars_ is either empty or contains only xlist
    if (kwargs.contains("rxlist")) {
        auto rxlist = extract_additional_vars_(kwargs["rxlist"]);

        if (!rxlist.empty()) {
            if (vars_.empty()) {
                vars_ = rxlist;
            } else {
                vars_.append(" ; ").append(rxlist);
            }
        }
    }

    // we build the libgretl list of endogenous and additional (xlist and/or rxlist) variables
    auto varlist_ptr = dataset_->gretllist(varlist);
    gretl_list_ptr additional_vars{nullptr};
    auto varlist_str = std::string(gretl_list_to_string(varlist_ptr.get(), dataset_->dset, &err_));

    if (err_) handle_gretl_error(err_);
    gretl4py_trim(varlist_str);

    // we build vars_, which is varlist + (xlist + rxlist), if exist
    if (!vars_.empty()) {
        additional_vars = dataset_->gretllist(vars_);
        if (!additional_vars) handle_gretl_error(E_UNKVAR);
        vars_ = varlist_str  + " ; " + vars_;
    } else {
        vars_ = varlist_str;
    }
    if (additional_vars) {
        auto joined = gretl_lists_join_with_separator(varlist_ptr.get(), additional_vars.get());

        if (!joined) handle_gretl_error(E_ALLOC);
        var_list_.reset(joined);
    } else {
        var_list_ = std::move(varlist_ptr);
    }
}

void GretlModel_VAR_VECM::fit (void)
{
    // we check if we deal with clean model
    check_if_estimated_();

    // we do the job
    model_ = gretl_VECM(order_, coint_rank_, var_list_.get(), dataset_->dset, opt_, printer_.prn_ptr(), &err_);
    if (err_) handle_gretl_error(err_);

    // save results
    save_model_<GRETL_VAR>(model_);
}

GretlBundlePtr GretlModel_VAR_VECM::restrict (const py::list &impose, const py::kwargs &kwargs)
{
    return do_restrict_<GRETL_VAR>(model_, impose, kwargs);
}

std::string GretlModel_VAR_VECM::save_ec_term (const int nr, const std::string &name) const
{
    //
    // here we add EC to dataset of number nr
    // Note: nr must be 1-based (not 0-based)
    //
    std::string ec_term_name;

    // guard for cases when in previous call we wanted ECM from non-existing equation
    if (err_ == E_DATA) err_ = 0;

    // we retrive ec_term
    gretl_double_ptr ec_term(gretl_VECM_get_EC(model_, nr-1, dataset_->dset, &err_));

    if (err_) handle_gretl_error(err_);

    // we set name
    if (name.empty()) ec_term_name = std::string("m") + std::to_string(id_) + std::string("_EC") + std::to_string(nr);

    // actual work
    return dataset_->add_series(std::move(ec_term), name.empty() ? ec_term_name : name, dataset_->dset->n);
}

py::object GretlModel_VAR_VECM::plot_ec (void)
{
    //
    // Plot the error-correction terms of an estimated VECM.
    //
    if (!estimated_) throw std::runtime_error("VECM model has not been estimated");

    GretlPlot plotter;

    // Register the output file for the EC plot.
    prepare_plot_(plotter, OPT_NONE);

    // Generate the combined EC plot using libgretl.
    err_ = gretl_VECM_combined_EC_plot(model_, dataset_->dset);
    if (err_) handle_gretl_error(err_);

    return plotter.display();
}

//
// Native Multiple Equation libgretl SYSTEM (NME)
// extends: GretlModel
//
GretlModel_NME::GretlModel_NME (const std::string &method, const py::kwargs &kwargs)
                                : GretlModel(GRETL_OBJ_SYS, SYSTEM, kwargs), method_(method)
{
    // we load system definition
    get_object_from_dict<std::vector<std::string>>(kwargs, "equations", equations_);
    if (equations_.empty()) throw std::runtime_error("no equations definitions passed");
    get_object_from_dict<std::vector<std::string>>(kwargs, "instruments", instruments_);
    get_object_from_dict<std::vector<std::string>>(kwargs, "identities", identities_);
    get_object_from_dict<std::string>(kwargs, "endogs", endogs_);
}

GretlModel_NME::~GretlModel_NME (void)
{
    gretl_object_unref(this->model_, this->model_obj_type_);
}

void GretlModel_NME::fit (void)
{
    //lambda for setting instruments_, identities_ and endogs
    auto do_system_parse_line = [&](const char *type, std::vector<std::string> &src)
    {
        if (src.empty()) return;
        std::string line;

        for (const auto &el : src) {
            line = std::string(type).append(" ").append(el);
            err_ = system_parse_line(model_, line.c_str(), dataset_->dset);
            if (err_) handle_gretl_error(err_);
        }
    };

    // we check if we deal with clean model
    check_if_estimated_();

    // system definition
    model_ = equation_system_start(std::string("method=").append(method_).data(), nullptr, opt_, &err_);
    if (err_) handle_gretl_error(err_);

    // we define equations
    for (const auto &el : equations_) {
        var_list_ = dataset_->gretllist(el);
        err_ = equation_system_append(model_, var_list_.get());
        if (err_) handle_gretl_error(err_);
    }

    // we define the rest of the system
    do_system_parse_line("instr", instruments_);
    do_system_parse_line("identity", identities_);
    if (endogs_.size()) {
        std::string line = "endog " + endogs_;

        err_ = system_parse_line(model_, line.c_str(), dataset_->dset);
        if (err_) handle_gretl_error(err_);
    }

    // do the job
    err_ = equation_system_finalize(model_, dataset_->dset, opt_, printer_.prn_ptr());
    if (err_) handle_gretl_error(err_);

    // we save model
    save_model_<equation_system>(model_);
}

std::string GretlModel_NME::get_formula (void) const
{
    py::print("GretlModel_NME::get_formula(): to be written");

    return "";
}

py::object GretlModel_NME::get_accessor (const std::string &name) const
{
    return do_get_accessor<GRETL_OBJ_SYS>(name, dataset_, static_cast<void*>(model_));
}

void GretlModel_NME::forecast (const py::args &args, const py::kwargs &kwargs)
{
    py::print("GretlModel_NME::forecast(): to be written");
}

GretlBundlePtr GretlModel_NME::restrict (const py::list &impose, const py::kwargs &kwargs)
{
    return do_restrict_<equation_system>(model_, impose, kwargs);
}

std::string GretlModel_NME::save_uhat (int eqn, const std::string &name) const
{
    return do_save_uhat_<equation_system>(model_, eqn, name);
}
