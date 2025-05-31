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
#include "gretl4py_model_instantiations.h"

extern std::shared_ptr<GretlDataset> DSET;

#define BIND_DRIVER_MODEL(NAME, FUNC, CI) \
    m.def(#NAME, \
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
          py::arg("val").none(true) = std::nullopt),
    m.def("new_matrix",
          [] (const std::string &formula, std::shared_ptr<GretlDataset> data)
          {
              do_generate<GRETL_TYPE_MATRIX>(formula, data ? data : DSET);
          },
          py::arg("formula").none(false), py::arg("data").none(true) = std::nullopt);

    //////////////////////////////////////////////////
    // single equation models
    //////////////////////////////////////////////////
    // direct calls
    m.def("arima",
          [](const std::string &y, const py::tuple &order, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_NSE_arima>(DSET, y, order, kwargs);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("y").none(false), py::arg("order").none(false), py::kw_only());
    m.def("quantreg",
          [](const double &tau, const std::string &formula, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_NSE_quantreg>(DSET, formula, tau, kwargs);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("tau").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("quantreg",
          [](const py::array_t<double> &tau, const std::string &formula, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_NSE_quantreg>(DSET, formula, tau, kwargs);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("tau").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("dpanel",
          [](const int &p, const std::string &formula, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_NSE_dpanel>(DSET, formula, p, kwargs);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("p").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("dpanel",
          [](const py::array_t<int> &p, const std::string &formula, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_NSE_dpanel>(DSET, formula, p, kwargs);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("p").none(false), py::arg("formula").none(false), py::kw_only());

    // estimation via wrappers
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
    BIND_DRIVER_MODEL(hsk, hsk_model, HSK)
    BIND_DRIVER_MODEL(intreg, interval_model, INTREG)
    BIND_DRIVER_MODEL(midasreg, midas_model, MIDASREG)

    //////////////////////////////////////////////////
    // VAR and VECM
    //////////////////////////////////////////////////
    m.def("var",
          [](const int &order, const std::string &ylist, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_VAR>(DSET, ylist, order, kwargs);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("order").none(false), py::arg("ylist").none(false), py::kw_only());
    m.def("vecm",
          [](const int &order, const int &rank, const std::string &ylist, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_VAR_VECM>(DSET, ylist, order, rank, kwargs);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("order").none(false), py::arg("rank").none(false),
          py::arg("ylist").none(false), py::kw_only());
}
