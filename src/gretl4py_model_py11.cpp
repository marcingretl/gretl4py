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
#include "gretl4py_model_instantiations.h"

namespace py = pybind11;

// template for NSE via dedicated Driver
// on Python side class will be registered under 'name'
template <typename T>
static void bind_driver_model(py::module_ &m, const std::string &name)
{
    py::class_<T, GretlModel_NSE, std::unique_ptr<T>>(m, name.c_str(), py::dynamic_attr());
}

void init_gretl4py_model_py11(py::module_ &m)
{
    // base GretlModel class
    py::class_<GretlModel, std::unique_ptr<GretlModel>>(m, "Model", py::dynamic_attr())
        .def_readwrite("tests", &GretlModel::tests)
        .def("fit",
             [](GretlModel &self) -> GretlModel&
             {
                 self.fit();

                 return self;
             })
        .def("get_accessor",
             [](GretlModel &self, const std::string &accessor) -> GretlTypes*
             {
                 return self.get_accessor(accessor).release();
             },
             py::return_value_policy::take_ownership)
        .def("fcast", &GretlModel::forecast)
        .def("get_formula", &GretlModel::get_formula)
        .def("restrict", &GretlModel::restrict)
        .def("test", py::overload_cast<const std::string&, const py::kwargs&>(&GretlModel::test),
             py::arg("type").none(false), py::kw_only())
        .def("test", py::overload_cast<const std::string&, const std::string&, const py::kwargs&>(&GretlModel::test),
             py::arg("type").none(false), py::arg("vars").none(false), py::kw_only());

    // Native Single Equation libgretl MODEL (NSE)
    py::class_<GretlModel_NSE, GretlModel, std::unique_ptr<GretlModel_NSE>>(m, "Model_NSE", py::dynamic_attr())
        .def("restrict", &GretlModel_NSE::restrict,
             py::arg("impose").none(false), py::kw_only())
        .def("test",
             [](GretlModel_NSE &self, const std::string &type, const py::kwargs &kwargs)
             {
                 //
                 // lambda rationale:
                 // 1. handles overload resolution cleanly (keyword vs positional)
                 // 2. special-cases "chow" which uses 'obs' instead of 'vars'
                 //
                 py::kwargs clean_kwargs = kwargs;
                 bool with_vars = false;
                 std::string vars;

                 // we check for "vars" or "obs" for chow test
                 if (kwargs.contains("vars")) {
                     vars = py::str(clean_kwargs.attr("pop")("vars"));
                     with_vars = true;
                 } else if (type == "chow" && kwargs.contains("obs")) {
                     vars = py::str(clean_kwargs.attr("pop")("obs"));
                     with_vars = true;
                 }

                 // lest perform test
                 with_vars ? self.test(type, vars, clean_kwargs) : self.test(type, clean_kwargs);
             },
             py::arg("type").none(false), py::kw_only());

    // NSE via arima plugin
    py::class_<GretlModel_NSE_arima, GretlModel_NSE,
        std::unique_ptr<GretlModel_NSE_arima>>(m, "Model_arima", py::dynamic_attr());

    // NSE_Driver_quantreg via quantreg_driver
    py::class_<GretlModel_NSE_quantreg, GretlModel_NSE,
        std::unique_ptr<GretlModel_NSE_quantreg>>(m, "Model_quantreg", py::dynamic_attr());

    // NSE via dedicated Driver
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
    bind_driver_model<GretlModel_NSE_Driver<tobit_model, TOBIT>>(m, "Model_tobit");

    // VAR
    py::class_<GretlModel_VAR, GretlModel, std::unique_ptr<GretlModel_VAR>>(m, "Model_var", py::dynamic_attr())
        .def("irf", py::overload_cast<int, int, double>(&GretlModel_VAR::irf),
             py::arg("targ").none(false), py::arg("shock").none(false), py::arg("alpha").none(true) = 0)
        .def("irf", py::overload_cast<const std::string&, const std::string&, double>(&GretlModel_VAR::irf),
             py::arg("targ").none(false), py::arg("shock").none(false), py::arg("alpha").none(true) = 0);
}
// py::object self = py::cast(model.get());
// py::gil_scoped_acquire gil;
// py::setattr(self, "res", res);
