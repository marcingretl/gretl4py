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
#include "gretl4py_bundle.h"
#include "gretl4py_matrix.h"
#include "gretl_calls.h"
#include "gretl4py_utils.h"
#include "gretl4py_model_instantiations.h"
#include "docstrings/gretl4py_model_docstr.h"
#include "docstrings/gretl4py_functions_docstr.h"

#define BIND_DRIVER_MODEL(NAME, FUNC, CI, DOC) \
    m.def(#NAME, \
          [](const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr \
          { \
              return std::make_unique<GretlModel_NSE_Driver<FUNC, CI>>(formula, kwargs); \
          }, \
          py::arg("formula").none(false), py::kw_only()); \
    m.def(#NAME, \
          [](const py::list &varlist, const py::kwargs &kwargs) -> GretlModelPtr \
          { \
              return std::make_unique<GretlModel_NSE_Driver<FUNC, CI>>(varlist, kwargs); \
          }, \
          py::arg("varlist").none(false), py::kw_only(), DOC);

constexpr char info[] =
    "The gretl4py package provides "
    "an interface to the gretl library for Python 3 "
    "(3.11--3.14 tested).";

//////////////////////////////////////////////////
// module extensions
//////////////////////////////////////////////////
void init_gretl4py_dataset_py11 (py::module_ &m);
void init_gretl4py_bundle_py11 (py::module_ &m);
void init_gretl4py_packages_py11 (py::module_ &m);
void init_gretl4py_model_py11 (py::module_ &m);
void init_gretl4py_gretl_matrix_py11 (py::module_ &m);

PYBIND11_MODULE(_gretl, m)
{
    // exceptions
    py::register_exception_translator ([](std::exception_ptr p)
    {
        try {
            if (p) std::rethrow_exception(p);
        } catch (const file_not_found &e) {
            PyErr_SetString(PyExc_FileNotFoundError, e.what());
        }
    });

    // gretl4py init/cleanup;
    gretl4py_init();
    py::module_::import("atexit").attr("register")(py::cpp_function([]() { gretl4py_cleanup(); }));

    init_gretl4py_dataset_py11(m);
    init_gretl4py_bundle_py11(m);
    init_gretl4py_packages_py11(m);
    init_gretl4py_model_py11(m);
    init_gretl4py_gretl_matrix_py11(m);

    // module related tools
    m.doc() = info;
    m.attr("version") = GRETL4PY_VERSION;
    m.attr("gretldir") = do_get_accessor<GRETL_OBJ_ANY>("$gretldir", GretlDataset::default_dataset(), nullptr);
    m.def("about", &about, py::arg("do_print") = false);

    // gretl4py hidden functions
    m.def("_get_data", &GretlDataset::get_data,
          py::arg("src").none(false), py::arg("source").none(true) = "", py::kw_only());
    m.def("_get_datasets_ids", &GretlDataset::get_dset_ids);

    // gretlpy public functions
    m.def("get_accessor",
          [](const std::string &name)
          {
              return do_get_accessor<GRETL_OBJ_ANY>(name, GretlDataset::default_dataset(), nullptr);
          },
          py::arg("name").none(false));
    m.def("get_uservar", &get_uservar,
          py::arg("varname").none(false));
    m.def("print", &do_gretl_print<void>,
          py::arg("varlist").none(true) = "", py::kw_only(), doc::Functions::print);
    m.def("run_script", &gretl_hansl_execute<true>,
          py::arg("filename").none(false));
    m.def("run_hansl", &gretl_hansl_execute<false>,
          py::arg("code").none(false));
    m.def("set",
          [](const std::string &variable, const std::string &value, const py::kwargs &kwargs)
          {
              do_set(variable, value, kwargs);
          },
          py::arg("variable").none(true) = "", py::arg("value").none(true) = "", py::kw_only());
    m.def("set",
          [](const std::string &variable, int value, const py::kwargs &kwargs)
          {
              do_set(variable, std::to_string(value), kwargs);
          },
          py::arg("variable").none(false), py::arg("value").none(false), py::kw_only());
    m.def("summary", py::overload_cast<const std::string&, const py::kwargs&>(&do_gretl_summary<std::string>),
          py::arg("varlist").none(true) = "", py::kw_only());
    m.def("summary", py::overload_cast<const py::list&, const py::kwargs&>(&do_gretl_summary<py::list>),
          py::arg("varlist").none(true) = py::list(), py::kw_only());
    m.def("summary",
          [](const GretlMatrixPtr &matrix, const py::kwargs &kwargs) -> py::object
          {
              return matrix->summary(kwargs);
          },
          py::arg("matrix").none(false), py::kw_only(), doc::Functions::summary);
    m.def("include", &do_gretl_include,
          py::arg("filename").none(false));
    m.def("delete",
          [](const std::string &varname, const py::kwargs &kwargs)
          {
              do_gretl_delete_variables(varname, kwargs);
          },
          py::arg("varname").none(true) = "", py::kw_only());
    m.def("pvalue", &gretl_get_pvalue_critval_pdf_cdf<GretlFunIndex::F_PVAL>,
          py::arg("c").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none());
    m.def("pvalue", &matrix_functions_wrapper<GretlFunIndex::F_PVAL, GretlMatrixPtr>,
          py::arg("c").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none());
    m.def("pvalue", &matrix_functions_wrapper<GretlFunIndex::F_PVAL, std::string>,
          py::arg("c").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none());
    m.def("pvalue", &matrix_functions_wrapper<GretlFunIndex::F_PVAL, py::sequence>,
          py::arg("c").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none(), doc::Functions::pvalue);
    m.def("critical", &gretl_get_pvalue_critval_pdf_cdf<GretlFunIndex::F_CRIT>,
          py::arg("c").none(false), py::arg("p").none(false), py::arg("params").none(true) = py::none());
    m.def("critical", &matrix_functions_wrapper<GretlFunIndex::F_CRIT, GretlMatrixPtr>,
          py::arg("c").none(false), py::arg("p").none(false), py::arg("params").none(true) = py::none());
    m.def("critical", &matrix_functions_wrapper<GretlFunIndex::F_CRIT, std::string>,
          py::arg("c").none(false), py::arg("p").none(false), py::arg("params").none(true) = py::none());
    m.def("critical", &matrix_functions_wrapper<GretlFunIndex::F_CRIT, py::sequence>,
          py::arg("c").none(false), py::arg("p").none(false), py::arg("params").none(true) = py::none(), doc::Functions::critical);
    m.def("pdf", &gretl_get_pvalue_critval_pdf_cdf<GretlFunIndex::F_PDF>,
          py::arg("d").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none());
    m.def("pdf", &matrix_functions_wrapper<GretlFunIndex::F_PDF, GretlMatrixPtr>,
          py::arg("d").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none());
    m.def("pdf", &matrix_functions_wrapper<GretlFunIndex::F_PDF, std::string>,
          py::arg("d").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none());
    m.def("pdf", &matrix_functions_wrapper<GretlFunIndex::F_PDF, py::sequence>,
          py::arg("d").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none(), doc::Functions::pdf);
    m.def("cdf", &gretl_get_pvalue_critval_pdf_cdf<GretlFunIndex::F_CDF>,
          py::arg("d").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none());
    m.def("cdf", &matrix_functions_wrapper<GretlFunIndex::F_CDF, GretlMatrixPtr>,
          py::arg("d").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none());
    m.def("cdf", &matrix_functions_wrapper<GretlFunIndex::F_CDF, std::string>,
          py::arg("d").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none());
    m.def("cdf", &matrix_functions_wrapper<GretlFunIndex::F_CDF, py::sequence>,
          py::arg("d").none(false), py::arg("x").none(false), py::arg("params").none(true) = py::none(), doc::Functions::cdf);
    m.def("invcdf", &gretl_get_pvalue_critval_pdf_cdf<GretlFunIndex::F_INVCDF>,
          py::arg("d").none(false), py::arg("u").none(false), py::arg("params").none(true) = py::none());
    m.def("invcdf", &matrix_functions_wrapper<GretlFunIndex::F_INVCDF, GretlMatrixPtr>,
          py::arg("d").none(false), py::arg("u").none(false), py::arg("params").none(true) = py::none());
    m.def("invcdf", &matrix_functions_wrapper<GretlFunIndex::F_INVCDF, std::string>,
          py::arg("d").none(false), py::arg("u").none(false), py::arg("params").none(true) = py::none());
    m.def("invcdf", &matrix_functions_wrapper<GretlFunIndex::F_INVCDF, py::sequence>,
          py::arg("d").none(false), py::arg("u").none(false), py::arg("params").none(true) = py::none(), doc::Functions::invcdf);
    m.def("cov", py::overload_cast<const py::array&, int>(&matrix_cov_corr<GretlFunIndex::F_MCOV>),
          py::arg("X").none(false), py::arg("dfcorr").none(true) = 1);
    m.def("cov", py::overload_cast<const GretlMatrixPtr&, int>(&matrix_cov_corr<GretlFunIndex::F_MCOV>),
          py::arg("X").none(false), py::arg("dfcorr").none(true) = 1, doc::Functions::cov);
    m.def("corr",
          [](const py::array &X)
          {
              return matrix_cov_corr<GretlFunIndex::F_MCORR>(X);
          },
          py::arg("X").none(false));
    m.def("corr",
          [](const GretlMatrixPtr X)
          {
              return matrix_cov_corr<GretlFunIndex::F_MCORR>(X);
          },
          py::arg("X").none(false), doc::Functions::corr);
    m.def("varlist", &do_gretl_varlist, doc::Functions::varlist);
    m.def("genr", static_cast<void(*)(const std::string&, GretlDatasetPtr)>(&genr),
          py::arg("formula").none(false), py::arg("data").none(true) = std::nullopt);
    m.def("genr", &genr<py::array>, py::arg("name").none(false), py::arg("value").none(false));
    m.def("genr", &genr<int>, py::arg("name").none(false), py::arg("value").none(false));
    m.def("genr", &genr<double>, py::arg("name").none(false), py::arg("value").none(false));
    m.def("genr", &genr<std::string>, py::arg("name").none(false), py::arg("value").none(false));
    m.def("genr", &genr<GretlMatrix>, py::arg("name").none(false), py::arg("value").none(false));
    m.def("pkg", &do_gretl_pkg,
          py::arg("action").none(false), py::arg("pkgname").none(false), py::kw_only());
    m.def("kmeans", py::overload_cast<const GretlMatrix&, int, const GretlBundlePtr>(&kmeans),
         py::arg("X").none(false), py::arg("centers").none(false), py::arg("opts").none(true) = nullptr);
    m.def("kmeans", py::overload_cast<const GretlMatrix&, const GretlMatrix&, const GretlBundlePtr>(&kmeans),
         py::arg("X").none(false), py::arg("centers").none(false), py::arg("opts").none(true) = nullptr, doc::Functions::kmeans);
    m.def("tdisagg",
          [](const GretlMatrix &Y, int s, const GretlMatrixPtr X, const py::kwargs &kwargs) -> GretlMatrixPtr
          {
              py::object y_obj = py::cast(Y);
              py::kwargs kwargs_copy = kwargs;

              if (X) kwargs_copy["X"] = X;

              return do_get_tdisagg_matrix(y_obj, s, kwargs_copy);
          },
          py::arg("Y").none(false), py::arg("s").none(false), py::arg("X").none(true) = nullptr, py::kw_only(), doc::Functions::tdisagg);
    m.def("kpsscrit", &do_kpsscrit,
          py::arg("T").none(false), py::arg("trend").none(true) = false, doc::Functions::kpsscrit);

    //////////////////////////////////////////////////
    // single equation models
    //////////////////////////////////////////////////
    // direct calls
    m.def("arima",
          [](const std::string &y, const py::tuple &order, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_arima>(y, order, kwargs);
          },
          py::arg("y").none(false), py::arg("order").none(false), py::kw_only(), doc::Model::arima);
    m.def("quantreg",
          [](double tau, const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_Driver_quantreg>(formula, tau, kwargs);
          },
          py::arg("tau").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("quantreg",
          [](const py::array_t<double> &tau, const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_Driver_quantreg>(formula, tau, kwargs);
          },
          py::arg("tau").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("quantreg",
          [](double tau, const py::list &varlist, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_Driver_quantreg>(varlist, tau, kwargs);
          },
          py::arg("tau").none(false), py::arg("varlist").none(false), py::kw_only());
    m.def("quantreg",
          [](const py::array_t<double> &tau, const py::list &varlist, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_Driver_quantreg>(varlist, tau, kwargs);
          },
          py::arg("tau").none(false), py::arg("varlist").none(false), py::kw_only(), doc::Model::quantreg);
    m.def("dpanel",
          [](int p, const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_Driver_dpanel>(formula, p, kwargs);
          },
          py::arg("p").none(false), py::arg("formula").none(false), py::kw_only());
    m.def("dpanel",
          [](const py::array_t<int> &p, const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_Driver_dpanel>(formula, p, kwargs);
          },
          py::arg("p").none(false), py::arg("formula").none(false), py::kw_only(), doc::Model::dpanel);
     m.def("tobit",
          [](const std::string &formula, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_Driver_tobit>(formula, kwargs);
          },
          py::arg("formula").none(false), py::kw_only());
     m.def("tobit",
           [](const py::list &varlist, const py::kwargs &kwargs) -> GretlModelPtr
           {
               return std::make_unique<GretlModel_NSE_Driver_tobit>(varlist, kwargs);
           },
           py::arg("varlist").none(false), py::kw_only(), doc::Model::tobit);
     m.def("_nls",
          [](const std::string &function, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_nl<NLS>>(function, kwargs);
          },
          py::arg("function").none(false), py::kw_only());
    m.def("_mle",
          [](const std::string &loglik, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_nl<MLE>>(loglik, kwargs);
          },
          py::arg("loglik").none(false), py::kw_only());
    m.def("_gmm",
          [](const std::string &resid, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NSE_nl<GMM>>(resid, kwargs);
          },
          py::arg("resid").none(false), py::kw_only());

    // estimation via wrappers
    BIND_DRIVER_MODEL(ols, lsq, OLS, doc::Model::ols)
    BIND_DRIVER_MODEL(wls, lsq, WLS, doc::Model::wls)
    BIND_DRIVER_MODEL(logit, logit_probit, LOGIT, doc::Model::logit)
    BIND_DRIVER_MODEL(probit, logit_probit, PROBIT, doc::Model::probit)
    BIND_DRIVER_MODEL(poisson, count_model, POISSON, doc::Model::poisson)
    BIND_DRIVER_MODEL(negbin, count_model, NEGBIN, doc::Model::negbin)
    BIND_DRIVER_MODEL(biprobit, biprobit_model, BIPROBIT, doc::Model::biprobit)
    BIND_DRIVER_MODEL(duration, duration_model, DURATION, doc::Model::duration)
    BIND_DRIVER_MODEL(heckit, heckit_model, HECKIT, doc::Model::heckit)
    BIND_DRIVER_MODEL(logistic, logistic_driver, LOGISTIC, doc::Model::logistic)
    BIND_DRIVER_MODEL(ar, ar_model, AR, doc::Model::ar)
    BIND_DRIVER_MODEL(ar1, ar1_model, AR1, doc::Model::ar1)
    BIND_DRIVER_MODEL(lad, lad_model, LAD, doc::Model::lad)
    BIND_DRIVER_MODEL(garch, garch, GARCH, doc::Model::garch)
    BIND_DRIVER_MODEL(mpols, mp_ols, MPOLS, doc::Model::mpols)
    BIND_DRIVER_MODEL(panel, panel_model, PANEL, doc::Model::panel)
    BIND_DRIVER_MODEL(tsls, ivreg, IVREG, doc::Model::tsls)
    BIND_DRIVER_MODEL(hsk, hsk_model, HSK, doc::Model::hsk)
    BIND_DRIVER_MODEL(intreg, interval_model, INTREG, doc::Model::intreg)
    BIND_DRIVER_MODEL(midasreg, midas_model, MIDASREG, doc::Model::midasreg)

    //////////////////////////////////////////////////
    // VAR and VECM
    //////////////////////////////////////////////////
    m.def("var",
          [](int order, const std::string &ylist, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_VAR>(ylist, order, VAR, kwargs);
          },
          py::arg("order").none(false), py::arg("ylist").none(false), py::kw_only());
    m.def("var",
          [](int order, const py::list &varlist, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_VAR>(varlist, order, VAR, kwargs);
          },
          py::arg("order").none(false), py::arg("varlist").none(false), py::kw_only(), doc::Model::var);
    m.def("vecm",
          [](int order, int rank, const std::string &ylist, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_VAR_VECM>(ylist, order, rank, kwargs);
          },
          py::arg("order").none(false), py::arg("rank").none(false),
          py::arg("ylist").none(false), py::kw_only());
    m.def("vecm",
          [](int order, int rank, py::list &varlist, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_VAR_VECM>(varlist, order, rank, kwargs);
          },
          py::arg("order").none(false), py::arg("rank").none(false),
          py::arg("varlist").none(false), py::kw_only(), doc::Model::vecm);

    //////////////////////////////////////////////////
    // SYSTEMs
    //////////////////////////////////////////////////
    m.def("system",
          [](const std::string &method, const py::kwargs &kwargs) -> GretlModelPtr
          {
              return std::make_unique<GretlModel_NME>(method, kwargs);
          },
          py::arg("method").none(false), py::kw_only(), doc::Model::system);

    /*
    //////////////////////////////////////////////////
    // packages
    //////////////////////////////////////////////////
    m.def("package",
          [](const std::string &name, const py::kwargs &kwargs) -> GretlModelPtr
          {
              auto model = std::make_unique<GretlModel_package>(name, kwargs);

              return GretlModelPtr(std::move(model));
          },
          py::arg("name").none(false), py::kw_only());*/
}
