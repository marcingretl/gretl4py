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

#define GRETL4PY_VERSION 0.22

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <gretl/libgretl.h>
extern "C" {
    #include <gretl/uservar.h>
}
#undef gettext
#include "gretl4py_types.h"
#include "gretl4py_data.h"
#include "gretl4py_model.h"

namespace py = pybind11;

//////////////////////////////////////////////////
// module related tools
//////////////////////////////////////////////////

void module_init (void);

void module_cleanup (void);

GretlBundlePtr about (bool do_print = false);

void do_set (const std::string key,
             const std::optional<py::object> &pyval = std::nullopt);

void do_summary (const std::string &varlist, const py::kwargs &kwargs);

double do_gretl_get_pvalue (const std::string &dist, double value,
                            const py::array_t<double> &params = py::array());

py::object do_include (const std::string &name);

// these two functions are temporal and will be moved to class
void gretl_user_var_delete_by_name (const std::string &name);

void gretl_print_user_var_by_name (const std::string &name,
                                   GretlDatasetPtr data = nullptr);

void uservars_list (void);

void genr (const std::string &formula, GretlDatasetPtr data);

template <typename T>
void genr (const std::string &name, const T &value);

#endif // GRETL4PY_H
