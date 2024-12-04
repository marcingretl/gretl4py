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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#define GRETL_MODULE_VERSION 0.1

namespace py = pybind11;

// module related tools
void module_init (void);

void module_cleanup (void);

py::dict about (bool do_print = false);

void set (const std::string key,
          const std::optional<py::object> &pyval = std::nullopt);

// data management
void attach_data (const py::dict &pydict);

void attach_gretl_data (const std::string fname);

py::dict *get_gretl_data (const std::string fname);

void printdata (const std::optional<py::dict> &data = std::nullopt,
                bool namesonly = false);

void sample (const py::kwargs &kwargs);

// model estimation
py::dict *arma (const std::string y, const py::tuple &tup, const py::kwargs &kwargs);

py::dict *ols (const std::string formula, const py::kwargs &kwargs);

py::dict *wls (const std::string formula, const py::kwargs &kwargs);

py::dict *logit (const std::string formula, const py::kwargs &kwargs);

py::dict *probit (const std::string formula, const py::kwargs &kwargs);

py::dict *poisson (const std::string formula, const py::kwargs &kwargs);

py::dict *negbin (const std::string formula, const py::kwargs &kwargs);

// content from automation/more_calls.h

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
