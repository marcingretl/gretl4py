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
template<typename T, typename Base = GretlModel_NSE>
static void bind_driver_model(py::module_ &m, const std::string &name)
{
    py::class_<T, Base, std::unique_ptr<T>>(m, name.c_str(), py::dynamic_attr());
}

void init_gretl4py_model_py11(py::module_ &m)
{
    // base GretlModel class
    py::class_<GretlModel, std::unique_ptr<GretlModel>>(m, "Model", py::dynamic_attr())
        .def("__repr__", &GretlModel::repr)
        .def("fit",
             [](GretlModel &self) -> GretlModel&
             {
                 self.fit();

                 return self;
             })
        .def("get_accessor", &GretlModel::get_accessor,
             py::arg("accessor").none(false),
             py::return_value_policy::take_ownership)
        .def("fcast", &GretlModel::forecast)
        .def("get_formula", &GretlModel::get_formula);

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

                 // we perform test
                 with_vars ? self.test(type, vars, clean_kwargs) : self.test(type, clean_kwargs);
             },
             py::arg("type").none(false), py::kw_only());

    // NSE via dedicated Driver
    bind_driver_model<GretlModel_NSE_arima>(m, "Model_arima");
    bind_driver_model<GretlModel_NSE_Driver_dpanel<dpd_model, DPANEL>>(m, "Model_dpanel");
    bind_driver_model<GretlModel_NSE_Driver_quantreg<quantreg_driver, QUANTREG>>(m, "Model_quantreg");
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
    bind_driver_model<GretlModel_NSE_Driver<hsk_model, HSK>>(m, "Model_hsk");
    bind_driver_model<GretlModel_NSE_Driver<interval_model, INTREG>>(m, "Model_intreg");
    bind_driver_model<GretlModel_NSE_Driver<midas_model, MIDASREG>>(m, "Model_midasreg");

    // VAR
    py::class_<GretlModel_VAR, GretlModel, std::unique_ptr<GretlModel_VAR>>(m, "Model_var", py::dynamic_attr())
        .def("test", &GretlModel_VAR::test,
             py::arg("type").none(false), py::kw_only())
        .def("irf", py::overload_cast<int, int, double>(&GretlModel_VAR::irf),
             py::arg("targ").none(false), py::arg("shock").none(false), py::arg("alpha").none(true) = 0)
        .def("irf", py::overload_cast<const std::string&, const std::string&, double>(&GretlModel_VAR::irf),
             py::arg("targ").none(false), py::arg("shock").none(false), py::arg("alpha").none(true) = 0);

    // VECM
    py::class_<GretlModel_VAR_VECM, GretlModel_VAR, std::unique_ptr<GretlModel_VAR_VECM>>(m, "Model_vecm", py::dynamic_attr())
        .def("restrict", &GretlModel_VAR_VECM::restrict,
             py::arg("impose").none(false), py::kw_only());

    // GretlModel_package
    py::class_<GretlModel_package, GretlModel, std::unique_ptr<GretlModel_package>>(m, "Model_package", py::dynamic_attr())
        .def("functions", &GretlModel_package::functions);
}
// py::object self = py::cast(model.get());
// py::gil_scoped_acquire gil;
// py::setattr(self, "res", res);
