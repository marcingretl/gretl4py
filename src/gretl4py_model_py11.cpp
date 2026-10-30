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
#include "gretl4py_bundle.h"
#include "gretl4py_matrix.h"
#include "gretl4py_model_instantiations.h"
#include "docstrings/gretl4py_model_docstr.h"

namespace py = pybind11;

namespace {
// wrappers for GretlModel_NSE_nl class to adding lines for parsing
enum NLLineType { PARAMS, PARAM_NAMES, WEIGHTS, DERIV, ORTHOG, STATEMENTS };

// template for NSE via dedicated Driver
// on Python side class will be registered under 'name'
template <typename T, typename Base = GretlModel_NSE>
void bind_driver_model (py::module_ &m, const std::string &name)
{
    auto cls = py::class_<T, Base, std::unique_ptr<T>>(m, name.c_str(), py::dynamic_attr());

    cls.attr("__module__") = "gretl";
}

template <GretlCmdIndex CI, NLLineType type>
GretlModel_NSE_nl<CI> &nl_add_line_to_parse (GretlModel_NSE_nl<CI> &model, const std::string &line)
{
    constexpr bool is_str = (type == PARAMS || type == PARAM_NAMES || type == WEIGHTS);

    constexpr const char *keyword =
        type == PARAMS      ? "params" :
        type == PARAM_NAMES ? "param_names" :
        type == WEIGHTS     ? "weights" :
        type == DERIV       ? "deriv" :
        type == ORTHOG      ? "orthog" :
                              "statements";

    model.template add_line_to_parse<is_str>(keyword, line);

    return model;
}

template <GretlCmdIndex CI>
GretlModel_NSE_nl<CI> &nl_add_init_value (GretlModel_NSE_nl<CI> &model, const std::string &varname, double value)
{
    model.add_init_value(varname, value);

    return model;
}
}

// GretlModel mappings
void init_gretl4py_model_py11(py::module_ &m)
{
    // base GretlModel class
    auto gretl_model = py::class_<GretlModel, GretlModelPtr>(m, "Model", py::dynamic_attr());

    gretl_model
    .def_property_readonly("dataset",
                           [](const GretlModel &self)
                           {
                               return self.get_dataset();
                           }, doc::Model::dataset)
    .def_property_readonly("estimated",
                           [](const GretlModel &self)
                           {
                               return self.estimated();
                           }, doc::Model::estimated)
    .def("__repr__", &GretlModel::repr_impl)
    .def("fit",
         [](GretlModel &self) -> GretlModel&
         {
             self.fit();

             return self;
         }, doc::Model::fit)
    .def("get_accessor", &GretlModel::get_accessor,
         py::arg("name").none(false), doc::Model::get_accessor)
    .def("fcast", &GretlModel::forecast, doc::Model::fcast)
    .def("get_formula", &GretlModel::get_formula, doc::Model::get_formula)
    .def("get_bundle", &GretlModel::get_bundle, doc::Model::get_bundle);
    gretl_model.attr("__module__") = "gretl";

    // Native Single Equation libgretl MODEL (NSE)
    auto gretl_model_nse = py::class_<GretlModel_NSE, GretlModel, std::unique_ptr<GretlModel_NSE>>(m, "Model_NSE", py::dynamic_attr());

    gretl_model_nse
    .def_property_readonly("adj_rsq", &GretlModel_NSE::adj_rsq, doc::Model::adj_rsq)
    .def("restrict", &GretlModel_NSE::restrict,
         py::arg("impose").none(false), py::kw_only(), doc::Model::restrict)
    .def("save_yhat", &GretlModel_NSE::gretl_get_fit_or_resid<M_YHAT>,
         py::arg("name").none(true) = "", doc::Model::save_yhat)
    .def("save_uhat", &GretlModel_NSE::gretl_get_fit_or_resid<M_UHAT>,
         py::arg("name").none(true) = "", doc::Model::save_uhat)
    .def("save_uhat_sq", &GretlModel_NSE::gretl_get_fit_or_resid<M_UHAT2>,
         py::arg("name").none(true) = "", doc::Model::save_uhat_sq)
    .def("save_ahat", &GretlModel_NSE::gretl_get_fit_or_resid<M_AHAT>,
         py::arg("name").none(true) = "", doc::Model::save_ahat)
    .def("save_h", &GretlModel_NSE::gretl_get_fit_or_resid<M_H>,
         py::arg("name").none(true) = "", doc::Model::save_h)
    .def("plot_new", &GretlModel_NSE::plot,
         py::arg("type").none(false), py::arg("xvar").none(true) = "")
    .def("_test",
         [](GretlModel_NSE &self, const std::string &type, const py::kwargs &kwargs) -> GretlBundlePtr
         {
             //
             // 1. handles overload resolution cleanly (keyword vs positional)
             // 2. special-cases: "chow" which uses 'obs' instead of 'varlist'
             //
             if (!self.estimated()) throw std::runtime_error("Model was not estimated yet");
             py::object varlist;
             py::kwargs clean_kwargs = kwargs;
             bool with_vars = false;

             // we check for "vars" or "obs" for chow test
             if (kwargs.contains("varlist")) {
                 varlist = clean_kwargs.attr("pop")("varlist");
                 with_vars = true;
             } else if (type == "chow" && kwargs.contains("obs")) {
                 varlist = clean_kwargs.attr("pop")("obs");
                 with_vars = true;
             }

             // we do the job
             return with_vars ? self.test(type, varlist, clean_kwargs) : self.test(type, clean_kwargs);
         }, py::arg("type").none(false), py::kw_only())
    .def("_add_auto",
         [](GretlModel_NSE &self, const py::kwargs &kwargs) -> py::tuple
         {
             auto [bundle, model] = self.add_omit_auto<ADD>(kwargs);

             return py::make_tuple(py::cast(bundle), py::cast(std::move(model)));
         })
    .def("_omit_auto",
         [](GretlModel_NSE &self, const py::kwargs &kwargs) -> py::tuple
         {
             auto [bundle, model] = self.add_omit_auto<OMIT>(kwargs);

             return py::make_tuple(py::cast(bundle), py::cast(std::move(model)));
         });
    gretl_model_nse.attr("__module__") = "gretl";

    // NSE via dedicated Driver
    bind_driver_model<GretlModel_NSE_arima>(m, "Model_arima");
    bind_driver_model<GretlModel_NSE_Driver<lsq, OLS>>(m, "Model_ols");
    bind_driver_model<GretlModel_NSE_Driver<lsq, WLS>>(m, "Model_wls");
    bind_driver_model<GretlModel_NSE_Driver<logit_probit, LOGIT>>(m, "Model_logit");
    bind_driver_model<GretlModel_NSE_Driver<logit_probit, PROBIT>>(m, "Model_probit");
    bind_driver_model<GretlModel_NSE_Driver<count_model, POISSON>>(m, "Model_poisson");
    bind_driver_model<GretlModel_NSE_Driver<count_model, NEGBIN>>(m, "Model_negbin");
    bind_driver_model<GretlModel_NSE_Driver<biprobit_model, BIPROBIT>>(m, "Model_biprobit");
    bind_driver_model<GretlModel_NSE_Driver<duration_model, DURATION>>(m, "Model_duration");
    bind_driver_model<GretlModel_NSE_Driver<heckit_model, HECKIT>>(m, "Model_heckit");
    bind_driver_model<GretlModel_NSE_Driver<logistic_driver, LOGISTIC>>(m, "Model_logistic");
    bind_driver_model<GretlModel_NSE_Driver<ar_model, AR>>(m, "Model_ar");
    bind_driver_model<GretlModel_NSE_Driver<ar1_model, AR1>>(m, "Model_ar1");
    bind_driver_model<GretlModel_NSE_Driver<lad_model, LAD>>(m, "Model_lad");
    bind_driver_model<GretlModel_NSE_Driver<garch, GARCH>>(m, "Model_garch");
    bind_driver_model<GretlModel_NSE_Driver<mp_ols, MPOLS>>(m, "Model_mpols");
    bind_driver_model<GretlModel_NSE_Driver<panel_model, PANEL>>(m, "Model_panel");
    bind_driver_model<GretlModel_NSE_Driver<ivreg, IVREG>>(m, "Model_ivreg");
    bind_driver_model<GretlModel_NSE_Driver<hsk_model, HSK>>(m, "Model_hsk");
    bind_driver_model<GretlModel_NSE_Driver<interval_model, INTREG>>(m, "Model_intreg");
    bind_driver_model<GretlModel_NSE_Driver<midas_model, MIDASREG>>(m, "Model_midasreg");
    bind_driver_model<GretlModel_NSE_Driver_quantreg>(m, "Model_quantreg");
    bind_driver_model<GretlModel_NSE_Driver_tobit>(m, "Model_tobit");
    bind_driver_model<GretlModel_NSE_Driver_dpanel>(m, "Model_dpanel");

    // OLS for resulting model of add/omit tests
    auto gretl_model_nse_ols_add =
    py::class_<GretlModel_NSE_OLS_add_omit<ADD>, GretlModel_NSE_Driver<lsq, OLS>,
        std::unique_ptr<GretlModel_NSE_OLS_add_omit<ADD>>>(m, "Model_ols_add", py::dynamic_attr());
    gretl_model_nse_ols_add.attr("__module__") = "gretl";

    auto gretl_model_nse_ols_omit =
    py::class_<GretlModel_NSE_OLS_add_omit<OMIT>, GretlModel_NSE_Driver<lsq, OLS>,
        std::unique_ptr<GretlModel_NSE_OLS_add_omit<OMIT>>>(m, "Model_ols_omit", py::dynamic_attr());
    gretl_model_nse_ols_omit.attr("__module__") = "gretl";

    // NL models (NLS/MLE/GMM)
    auto gretl_model_nse_nl_nls =
    py::class_<GretlModel_NSE_nl<NLS>, GretlModel_NSE, std::unique_ptr<GretlModel_NSE_nl<NLS>>>(m, "Model_nls", py::dynamic_attr());

    gretl_model_nse_nl_nls
    .def("add_params", &nl_add_line_to_parse<NLS, PARAMS>, doc::Model::nls_add_params)
    .def("add_param_names", &nl_add_line_to_parse<NLS, PARAM_NAMES>, doc::Model::add_param_names)
    .def("add_deriv", &nl_add_line_to_parse<NLS, DERIV>, doc::Model::nls_add_deriv)
    .def("add_initval", &nl_add_init_value<NLS>,
         py::arg("varname").none(false), py::arg("value").none(false), doc::Model::nls_add_initval);
    gretl_model_nse_nl_nls.attr("__module__") = "gretl";

    auto gretl_model_nse_nl_mle =
    py::class_<GretlModel_NSE_nl<MLE>, GretlModel_NSE, std::unique_ptr<GretlModel_NSE_nl<MLE>>>(m, "Model_mle", py::dynamic_attr());

    gretl_model_nse_nl_mle
    .def("add_params", &nl_add_line_to_parse<MLE, PARAMS>, doc::Model::mle_add_params)
    .def("add_param_names", &nl_add_line_to_parse<MLE, PARAM_NAMES>, doc::Model::add_param_names)
    .def("add_deriv", &nl_add_line_to_parse<MLE, DERIV>, doc::Model::mle_add_deriv)
    .def("add_statements", &nl_add_line_to_parse<MLE, STATEMENTS>, doc::Model::mle_add_statements)
    .def("add_initval", &nl_add_init_value<MLE>,
         py::arg("varname").none(false), py::arg("value").none(false), doc::Model::mle_add_initval);
    gretl_model_nse_nl_mle.attr("__module__") = "gretl";

    auto gretl_model_nse_nl_gmm =
    py::class_<GretlModel_NSE_nl<GMM>, GretlModel_NSE, std::unique_ptr<GretlModel_NSE_nl<GMM>>>(m, "Model_gmm", py::dynamic_attr());

    gretl_model_nse_nl_gmm
    .def("add_params", &nl_add_line_to_parse<GMM, PARAMS>, doc::Model::gmm_add_params)
    .def("add_param_names", &nl_add_line_to_parse<GMM, PARAM_NAMES>, doc::Model::add_param_names)
    .def("add_weights", &nl_add_line_to_parse<GMM, WEIGHTS>, doc::Model::gmm_add_weights)
    .def("add_orthog", &nl_add_line_to_parse<GMM, ORTHOG>, doc::Model::gmm_add_orthog)
    .def("add_statements", &nl_add_line_to_parse<GMM, STATEMENTS>, doc::Model::gmm_add_statements)
    .def("add_initval", &nl_add_init_value<GMM>,
         py::arg("varname").none(false), py::arg("value").none(false), doc::Model::gmm_add_initval);
    gretl_model_nse_nl_gmm.attr("__module__") = "gretl";

    // VAR
    auto gretl_model_var = py::class_<GretlModel_VAR, GretlModel, std::unique_ptr<GretlModel_VAR>>(m, "Model_var", py::dynamic_attr());

    gretl_model_var
    .def("_test", &GretlModel_VAR::test,
         py::arg("type").none(false), py::kw_only())
    .def("irf", py::overload_cast<int, int, double>(&GretlModel_VAR::irf),
         py::arg("target").none(false), py::arg("shock").none(false),
         py::arg("alpha").none(true) = 0)
    .def("irf", py::overload_cast<const std::string&, const std::string&, double>(&GretlModel_VAR::irf),
         py::arg("target").none(false), py::arg("shock").none(false),
         py::arg("alpha").none(true) = 0, doc::Model::irf)
    .def("save_uhat", &GretlModel_VAR::save_uhat,
         py::arg("eqn").none(false), py::arg("name").none(true) = "", doc::Model::save_system_uhat)
    .def("plot_new",
         [](GretlModel_VAR &self, const std::string &type, int equation) -> py::object
         {
             return self.plot(type, equation, "combined");
         }, py::arg("type").none(false), py::arg("equation") = 0)
    .def("plot_new",
         [](GretlModel_VAR &self, const std::string &type, const std::string &mode) -> py::object
         {
             return self.plot(type, 0, mode);
         }, py::arg("type").none(false), py::arg("mode").none(false));
    gretl_model_var.attr("__module__") = "gretl";

    // VECM
    auto gretl_model_var_vecm =
    py::class_<GretlModel_VAR_VECM, GretlModel_VAR, std::unique_ptr<GretlModel_VAR_VECM>>(m, "Model_vecm", py::dynamic_attr());

    gretl_model_var_vecm
    .def("restrict", &GretlModel_VAR_VECM::restrict,
         py::arg("impose").none(false), py::kw_only(), doc::Model::restrict)
    .def("save_ec_term", &GretlModel_VAR_VECM::save_ec_term,
         py::arg("nr").none(false), py::arg("name").none(true) = "", doc::Model::save_ec_term);
    gretl_model_var_vecm.attr("__module__") = "gretl";

    // Native Multiple Equation libgretl SYSTEM (NME) GretlModel_NME
    auto gretl_model_nme = py::class_<GretlModel_NME, GretlModel, std::unique_ptr<GretlModel_NME>>(m, "System", py::dynamic_attr());

    gretl_model_nme
    .def("restrict", &GretlModel_NME::restrict,
         py::arg("impose").none(false), py::kw_only(), doc::Model::restrict)
    .def("save_uhat", &GretlModel_NME::save_uhat,
         py::arg("eqn").none(false), py::arg("name").none(true) = "", doc::Model::save_system_uhat);
    gretl_model_nme.attr("__module__") = "gretl";
}
