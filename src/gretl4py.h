/*
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  gretl4py -- Python bindings for gretl
 *  Copyright (C) 2024 Marcin Błażejowski
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

#define HAVE_MPI

#include "gretl4py_version.h"
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <gretl/libgretl.h>
extern "C" {
    #include <gretl/uservar.h>
}
#include "gretl4py_types.h"
#include "gretl4py_dataset.h"
#include "gretl4py_model.h"

#define LIBGRETL_MIN_VER 57

#define GRETL_PYBIND11_DEBUG 0

namespace py = pybind11;

//////////////////////////////////////////////////
// module related tools
//////////////////////////////////////////////////
inline std::string gretl4py_logo()
{
    return R"(                _   _   ___
               | | | | /   |
  __ _ _ __ ___| |_| |/ /| |_ __  _   _
 / _` | '__/ _ \ __| / /_| | '_ \| | | |
| (_| | | |  __/ |_| \___  | |_) | |_| |
 \__, |_|  \___|\__|_|   |_/ .__/ \__, |
  __/ |                    | |     __/ |
 |___/                     |_|    |___/
)";
}

void gretl4py_init (void);

void gretl4py_cleanup (void);

GretlBundlePtr about (bool do_print = false);

#endif // GRETL4PY_H
