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

#ifndef GRETL4PY_H
#define GRETL4PY_H

#define GRETL4PY_VERSION 0.11

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "gretl4py_types.h"

namespace py = pybind11;

//////////////////////////////////////////////////
// module related tools
//////////////////////////////////////////////////

void module_init (void);

void module_cleanup (void);

py::dict about (bool do_print = false);

void set (const std::string key,
          const std::optional<py::object> &pyval = std::nullopt);

void GretlModel_destructor_wrapper (const int model_id);

void GretlModel_fcast (const int model_id, const py::kwargs &kwargs);

//////////////////////////////////////////////////
// model estimation
//////////////////////////////////////////////////
py::dict *arima (const std::string y, const py::tuple &tup, const py::kwargs &kwargs);

py::dict *ols (const std::string formula, const py::kwargs &kwargs);

py::dict *wls (const std::string formula, const py::kwargs &kwargs);

py::dict *logit (const std::string formula, const py::kwargs &kwargs);

py::dict *probit (const std::string formula, const py::kwargs &kwargs);

py::dict *poisson (const std::string formula, const py::kwargs &kwargs);

py::dict *negbin (const std::string formula, const py::kwargs &kwargs);

py::dict *biprobit (const std::string formula, const py::kwargs &kwargs);

py::dict *duration (const std::string formula, const py::kwargs &kwargs);

py::dict *heckit (const std::string formula, const py::kwargs &kwargs);

py::dict *logistic (const std::string formula, const py::kwargs &kwargs);

py::dict *ar (const std::string formula, const py::kwargs &kwargs);

py::dict *ar1 (const std::string formula, const py::kwargs &kwargs);

py::dict *lad (const std::string formula, const py::kwargs &kwargs);

py::dict *garch (const std::string formula, const py::kwargs &kwargs);

py::dict *mpols (const std::string formula, const py::kwargs &kwargs);

py::dict *panel (const std::string formula, const py::kwargs &kwargs);

py::dict *tsls (const std::string formula, const py::kwargs &kwargs);

py::dict *tobit (const std::string formula, const py::kwargs &kwargs);

py::dict *quantreg (const double tau, const std::string formula, const py::kwargs &kwargs);

py::dict *quantreg (const py::list tau, const std::string formula, const py::kwargs &kwargs);

#endif // GRETL4PY_H
