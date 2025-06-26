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

#ifndef GRETL4PY_TYPES_H
#define GRETL4PY_TYPES_H

// Ensure pybind11 and libgretl are included wherever this header is used
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <gretl/libgretl.h>
#include <gretl/gretl_drivers.h>

// Workaround for gettext redefinition
#undef gettext

namespace py = pybind11;

//////////////////////////////////////////////////
// types
//////////////////////////////////////////////////
using DNumPyArrayFC = py::array_t<double, py::array::f_style>;  // double contiguous column-major (fortran) NumPy array (F_CONTIGUOUS)

using GretlTypes = std::variant<double, std::string, DNumPyArrayFC, py::dict, py::list, py::none>;

using vgretl_vector = std::vector<std::variant<double, std::string, int>>;

using vgretl_matrix = std::vector<vgretl_vector>;

struct NumPyArrayView {
    void *data = nullptr;   // untyped pointer
    py::dtype dtype;        // numpy dtype info
    py::array base_array;   // holds ownership
    bool is_double = false; // for fast path
    bool NumPyArrayFC = false;
    int rows = 0;
    int cols = 0;
    // note strides in elements (not bytes!)
    ssize_t row_stride = 0;
    ssize_t col_stride = 0;
};

// gretl_list_ptr default deleter
struct GretlListDeleter {
    void operator() (int *ptr) const
    {
        #if defined(_WIN32)
            gretl_list_free(ptr);
        #else
            std::free(ptr);
        #endif
    }
};
using gretl_list_ptr = std::unique_ptr<int, GretlListDeleter>;

#endif // GRETL4PY_TYPES_H
