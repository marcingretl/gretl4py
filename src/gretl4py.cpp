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

#include "gretl4py.h"

extern std::shared_ptr<GretlDataset> DSET;
extern std::unordered_map<int, std::unique_ptr<GretlModel>> estimated_models;

#define BIND_DRIVER_MODEL(NAME, FUNC, CI) \
    m.def(#NAME "2", \
          [](const std::string &formula, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel> \
          { \
              auto model = std::make_unique<GretlModel_NSE_Driver<FUNC, CI>>(DSET, formula, kwargs); \
              return std::unique_ptr<GretlModel>(std::move(model)); \
          }, \
          py::arg("formula").none(false), py::kw_only());

class Module
{
public:
    Module() { module_init(); }
    ~Module() { module_cleanup(); }
};

const std::string info = "The gretl4py package provides "
    "an interface to the gretl library for python3 "
    "(3.11, 3.12, and 3.13 were tested).";

//////////////////////////////////////////////////
// module extensions
//////////////////////////////////////////////////
void init_gretl4py_data_py11(py::module_ &);
void init_gretl4py_model_py11(py::module_ &);


PYBIND11_MODULE(_gretl, m)
{
    static Module module;

    init_gretl4py_data_py11(m);
    init_gretl4py_model_py11(m);

    // module related tools
    m.doc() = info;
    m.attr("version") = GRETL4PY_VERSION;
    m.def("about", &about,
          py::arg("do_print") = false);
    m.def("set", &set,
          py::arg("key").none(false),
          py::arg("val").none(true) = std::nullopt);

    // model management
    m.def("GretlModel_destructor_wrapper", &GretlModel_destructor_wrapper,
          py::arg("model_id").none(false),
          py::arg("unlink") = true);
    m.def("get_accessor", &GretlModel_get_accessor,
          py::arg("model_id").none(false),
          py::arg("accessor").none(false),
          py::return_value_policy::take_ownership);
    m.def("test",
          py::overload_cast<const int, const std::string&,
          const py::kwargs&>(&GretlModel_test),
          py::arg("model_id").none(false),
          py::arg("type").none(false),
          py::return_value_policy::take_ownership);
    m.def("test",
          py::overload_cast<const int, const std::string&,
          const std::string&, const py::kwargs&>(&GretlModel_test),
          py::arg("model_id").none(false),
          py::arg("type").none(false),
          py::arg("vars").none(false),
          py::return_value_policy::take_ownership);
    m.def("restrict", &GretlModel_restrict,
          py::arg("model_id").none(false),
          py::arg("impose").none(false),
          py::return_value_policy::take_ownership);
    m.def("fcast", &GretlModel_fcast,
          py::arg("model_id").none(false),
          py::return_value_policy::take_ownership);

    // model estimation
    // estimation via wrappers
    m.def("ols",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<lsq, OLS>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("wls",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<lsq, WLS>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("logit",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<logit_probit, LOGIT>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("probit",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<logit_probit, PROBIT>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("poisson",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<count_model, POISSON>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("negbin",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<count_model, NEGBIN>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("biprobit",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<biprobit_model, BIPROBIT>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("duration",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<duration_model, DURATION>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("heckit",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<heckit_model, HECKIT>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("logistic",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<logistic_driver, LOGISTIC>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("ar",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<ar_model, AR>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("ar1",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<ar1_model, AR1>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("lad",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<lad_model, LAD>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("garch",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<garch, GARCH>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("mpols",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<mp_ols, MPOLS>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("panel",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<panel_model, PANEL>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("tsls",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<ivreg, IVREG>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("tobit",
          [](const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              return GretlModel_NSE_Driver_wrapper<tobit_model, TOBIT>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    // direct calls
    m.def("arima",
          [](const std::string &y, const py::tuple &order, const py::kwargs &kwargs) -> py::dict
          {
              auto model = std::make_unique<GretlModel_NSE_arima>(DSET, y, order, py::kwargs());

              py::dict res = model->fit("", kwargs);
              estimated_models.try_emplace(model->get_id(), std::move(model));

              return res;
          },
          py::arg("y").none(false), py::arg("order").none(false), py::kw_only(),
          py::return_value_policy::take_ownership);
    m.def("quantreg",
          [](const double &tau, const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              auto model = std::make_unique<GretlModel_NSE_Driver_quantreg<quantreg_driver, QUANTREG>>(DSET, formula, tau, kwargs);

              py::dict res = model->fit(formula, kwargs);
              estimated_models.try_emplace(model->get_id(), std::move(model));

              return res;
          },
          py::arg("tau").none(false), py::arg("formula").none(false),
          py::kw_only(), py::return_value_policy::take_ownership);
    m.def("quantreg",
          [](const py::array_t<double> &tau, const std::string &formula, const py::kwargs &kwargs) -> py::dict
          {
              auto model = std::make_unique<GretlModel_NSE_Driver_quantreg<quantreg_driver, QUANTREG>>(DSET, formula, tau, kwargs);

              py::dict res = model->fit(formula, kwargs);
              estimated_models.try_emplace(model->get_id(), std::move(model));

              return res;
          },
          py::arg("tau").none(false), py::arg("formula").none(false),
          py::kw_only(), py::return_value_policy::take_ownership);

// NEW definitions (for mapped class)
    m.def("arima2",
          [](const std::string &y, const py::tuple &order, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_NSE_arima>(DSET, y, order, kwargs);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("y").none(false), py::arg("order").none(false), py::kw_only());
    m.def("quantreg2",
          [](const double &tau, const std::string &formula, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_NSE_quantreg>(DSET, formula, tau, kwargs);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("tau").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("quantreg2",
          [](const py::array_t<double> &tau, const std::string &formula, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_NSE_quantreg>(DSET, formula, tau, kwargs);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("tau").none(false), py::arg("formula").none(false), py::kw_only());

    BIND_DRIVER_MODEL(ols, lsq, OLS)
    BIND_DRIVER_MODEL(wls, lsq, WLS)
    BIND_DRIVER_MODEL(logit, logit_probit, LOGIT)
    BIND_DRIVER_MODEL(probit, logit_probit, PROBIT)
    BIND_DRIVER_MODEL(poisson, count_model, POISSON)
    BIND_DRIVER_MODEL(negbin, count_model, NEGBIN)
    BIND_DRIVER_MODEL(biprobit, biprobit_model, BIPROBIT)
    BIND_DRIVER_MODEL(duration, duration_model, DURATION)
    BIND_DRIVER_MODEL(heckit, heckit_model, HECKIT)
    BIND_DRIVER_MODEL(logistic, logistic_driver, LOGISTIC)
    BIND_DRIVER_MODEL(ar, ar_model, AR)
    BIND_DRIVER_MODEL(ar1, ar1_model, AR1)
    BIND_DRIVER_MODEL(lad, lad_model, LAD)
    BIND_DRIVER_MODEL(garch, garch, GARCH)
    BIND_DRIVER_MODEL(mpols, mp_ols, MPOLS)
    BIND_DRIVER_MODEL(panel, panel_model, PANEL)
    BIND_DRIVER_MODEL(tsls, ivreg, IVREG)
    BIND_DRIVER_MODEL(tobit, tobit_model, TOBIT)
}
