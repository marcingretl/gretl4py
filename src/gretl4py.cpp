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

extern GretlDatasetPtr DSET;

#define BIND_DRIVER_MODEL(NAME, FUNC, CI) \
    m.def(#NAME, \
          [](const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr \
          { \
              auto model = std::make_unique<GretlModel_NSE_Driver<FUNC, CI>>(DSET, formula, kwargs); \
              return GretlModelPtr(std::move(model)); \
          }, \
          py::arg("formula").none(false), py::kw_only());

const std::string info = "The gretl4py package provides "
    "an interface to the gretl library for python3 "
    "(3.11, 3.12, and 3.13 were tested).";

//////////////////////////////////////////////////
// module extensions
//////////////////////////////////////////////////
void init_gretl4py_dataset_py11(py::module_ &);
void init_gretl4py_bundle_py11(py::module_ &);
void init_gretl4py_package_py11(py::module_ &);
void init_gretl4py_model_py11(py::module_ &);


PYBIND11_MODULE(_gretl, m)
{
    // gretl4py init/cleanup;
    gretl4py_init();
    m.def("_gretl4py_cleanup", []() { gretl4py_cleanup(); });

    init_gretl4py_dataset_py11(m);
    init_gretl4py_bundle_py11(m);
    init_gretl4py_package_py11(m);
    init_gretl4py_model_py11(m);

    // module related tools
    m.doc() = info;
    m.attr("version") = GRETL4PY_VERSION;
    m.def("about", &about,
          py::arg("do_print") = false);
    m.def("set", &do_set,
          py::arg("key").none(true) = "",
          py::arg("val").none(true) = std::nullopt);
    m.def("summary", &do_summary,
          py::arg("varlist").none(true) = "", py::kw_only());
    m.def("include", &do_include,
          py::arg("name").none(false));
    m.def("delete", &gretl_user_var_delete_by_name,
          py::arg("name").none(false));
    m.def("pvalue", &gretl_get_pvalue_critval_pdf_cdf<1>,
          py::arg("dist").none(false),
          py::arg("value").none(false),
          py::arg("params").none(true) = py::none());
    m.def("critical", &gretl_get_pvalue_critval_pdf_cdf<2>,
          py::arg("dist").none(false),
          py::arg("value").none(false),
          py::arg("params").none(true) = py::none());
    m.def("pdf", &gretl_get_pvalue_critval_pdf_cdf<3>,
          py::arg("dist").none(false),
          py::arg("value").none(false),
          py::arg("params").none(true) = py::none());
    m.def("cdf", &gretl_get_pvalue_critval_pdf_cdf<4>,
          py::arg("dist").none(false),
          py::arg("value").none(false),
          py::arg("params").none(true) = py::none());
    m.def("invcdf", &gretl_get_pvalue_critval_pdf_cdf<5>,
          py::arg("dist").none(false),
          py::arg("value").none(false),
          py::arg("params").none(true) = py::none());
    m.def("uservar_print", &gretl_print_user_var_by_name,
          py::arg("name").none(false), py::arg("data").none(true) = std::nullopt);
    m.def("uservars_list", &uservars_list);
    m.def("genr",
          static_cast<void(*)(const std::string&, GretlDatasetPtr)>(&genr),
          py::arg("formula").none(false), py::arg("data").none(true) = std::nullopt);
    m.def("genr", &genr<py::array>, py::arg("name").none(false), py::arg("value").none(false));
    m.def("genr", &genr<int>, py::arg("name").none(false), py::arg("value").none(false));
    m.def("genr", &genr<double>, py::arg("name").none(false), py::arg("value").none(false));
    m.def("genr", &genr<std::string>, py::arg("name").none(false), py::arg("value").none(false));

    //////////////////////////////////////////////////
    // single equation models
    //////////////////////////////////////////////////
    // direct calls
    m.def("arima",
          [](const std::string &y, const py::tuple &order, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_NSE_arima>(DSET, y, order, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("y").none(false), py::arg("order").none(false), py::kw_only());
    m.def("quantreg",
          [](const double &tau, const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_NSE_quantreg>(DSET, formula, tau, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("tau").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("quantreg",
          [](const py::array_t<double> &tau, const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_NSE_quantreg>(DSET, formula, tau, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("tau").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("dpanel",
          [](const int &p, const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_NSE_dpanel>(DSET, formula, p, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("p").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("dpanel",
          [](const py::array_t<int> &p, const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_NSE_dpanel>(DSET, formula, p, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("p").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("_nls",
          [](const std::string &function, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_NSE_nl<NLS>>(DSET, function, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("function").none(false), py::kw_only());
    m.def("_mle",
          [](const std::string &loglik, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_NSE_nl<MLE>>(DSET, loglik, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("loglik").none(false), py::kw_only());
    m.def("_gmm",
          [](const std::string &resid, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_NSE_nl<GMM>>(DSET, resid, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("resid").none(false), py::kw_only());

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
          [](const int &order, const std::string &ylist, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_VAR>(DSET, ylist, order, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("order").none(false), py::arg("ylist").none(false), py::kw_only());
    m.def("vecm",
          [](const int &order, const int &rank, const std::string &ylist, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_VAR_VECM>(DSET, ylist, order, rank, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("order").none(false), py::arg("rank").none(false),
          py::arg("ylist").none(false), py::kw_only());

    //////////////////////////////////////////////////
    // SYSTEMs
    //////////////////////////////////////////////////
    m.def("system",
          [](const std::string &method, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_NME>(DSET, method, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("method").none(false), py::kw_only());

    //////////////////////////////////////////////////
    // packages
    //////////////////////////////////////////////////
    m.def("package",
          [](const std::string &name, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_package>(DSET, name, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("name").none(false), py::kw_only());
}
