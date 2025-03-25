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
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

class Module
{
public:
    Module() { module_init(); }
    ~Module() { module_cleanup(); }
};

const std::string info = "The gretlpy package is a software collection "
    "that provides an interface to the libgretl for the python3.xx "
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
    m.attr("version") = GRETL4PY_VERSION;           // to changelog ; to doc
    m.def("about", &about,
          py::arg("do_print") = false);
    m.def("set", &set,
          py::arg("key").none(false),
          py::arg("val").none(true) = std::nullopt);

    // data management
    m.def("attach_data", &attach_data,
          py::arg("pydict").none(false));
    m.def("attach_gretl_data", &attach_gretl_data,
          py::arg("fname").none(false));
    m.def("get_gretl_data", &get_gretl_data,
          py::arg("fname").none(false));
    m.def("print_data", &printdata, // 2025-03-19: this is due to the style guidelines contained in PEP 8 (snake case)
          py::arg("data").none(true) = std::nullopt,
          py::arg("namesonly") = false);
    m.def("show_data_status", &show_data_status);

    // sampling (for gretl datasets only)
    m.def("sample", &sample);

    // model estimation
    // (S)ARIMA
    m.def("arima", &arima,
          py::arg("y").none(false),
          py::arg("order").none(false),
          py::return_value_policy::take_ownership);
    // OLS
    m.def("ols", &ols,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    // WLS
    m.def("wls", &wls,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    // Logit and probit
    m.def("logit", &logit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("probit", &probit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    // Count models
    m.def("poisson", &poisson,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("negbin", &negbin,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    // content from automation/more_mod.cpp
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
}
