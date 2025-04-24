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

PYBIND11_MODULE(_gretl, m)
{
    static Module module;

    init_gretl4py_data_py11(m);

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
          py::arg("model_id").none(false));

    // model estimation
    m.def("arima", &arima,
          py::arg("y").none(false),
          py::arg("order").none(false),
          py::return_value_policy::take_ownership);
    m.def("ols", &ols,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("wls", &wls,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("logit", &logit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("probit", &probit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("poisson", &poisson,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("negbin", &negbin,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("biprobit", &biprobit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("duration", &duration,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("heckit", &heckit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("logistic", &logistic,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("ar", &ar,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("ar1", &ar1,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("lad", &lad,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("garch", &garch,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("mpols", &mpols,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("panel", &panel,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("tsls", &tsls,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("tobit", &tobit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("quantreg",
          py::overload_cast<const py::list, const std::string,
          const py::kwargs&>(&quantreg),
          py::arg("tau").none(false),
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("quantreg",
          py::overload_cast<const double, const std::string,
          const py::kwargs&>(&quantreg),
          py::arg("tau").none(false),
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
}
