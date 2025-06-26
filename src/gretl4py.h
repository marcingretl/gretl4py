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

#define GRETL4PY_VERSION 0.21

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

py::dict about (bool do_print = false);

void set (const std::string key,
          const std::optional<py::object> &pyval = std::nullopt);

void gretl_user_var_delete_by_name (const std::string &name);

void gretl_print_user_var_by_name (const std::string &name,
                                   std::shared_ptr<GretlDataset> data = nullptr);

void uservars_list (void);

void new_matrix (const std::string &formula, std::shared_ptr<GretlDataset> data);

void new_matrix (const std::string &name, const std::string &value,
                 std::shared_ptr<GretlDataset> data);

void new_matrix (const std::string &name, const py::array &nparray);

#endif // GRETL4PY_H
