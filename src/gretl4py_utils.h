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

#ifndef GRETL4PY_UTILS_H
#define GRETL4PY_UTILS_H

#include "gretl4py_types.h"

namespace py = pybind11;

//////////////////////////////////////////////////
// macros
//////////////////////////////////////////////////
#define IS_PYGRETLBUNDLE(o) (py::isinstance<GretlBundle>(o))
#define SET_OPT(opt, flag) opt = static_cast<gretlopt>((opt) | (flag))

//////////////////////////////////////////////////
// exceptions
//////////////////////////////////////////////////
struct file_not_found : std::exception {
    //
    // FileNotFoundError
    //
    std::string msg;

    explicit file_not_found (const std::string &f) : msg(f) {}
    const char *what() const noexcept override { return msg.c_str(); }
};

//////////////////////////////////////////////////
// inline functions
//////////////////////////////////////////////////
// Python type checks
inline bool is_pybool (const py::handle &o)    { return py::isinstance<py::bool_>(o); }
inline bool is_pyint (const py::handle &o)     { return py::isinstance<py::int_>(o); }
inline bool is_pyfloat (const py::handle &o)   { return py::isinstance<py::float_>(o); }
inline bool is_pystr (const py::handle &o)     { return py::isinstance<py::str>(o); }
inline bool is_pydict (const py::handle &o)    { return py::isinstance<py::dict>(o); }
inline bool is_pytuple (const py::handle &o)   { return py::isinstance<py::tuple>(o); }
inline bool is_pylist (const py::handle &o)    { return py::isinstance<py::list>(o); }
inline bool is_pynumarray (const py::handle &o){ return py::isinstance<py::array>(o); }
inline bool is_pynumarrayd (const py::handle &o){ return py::isinstance<py::array_t<double>>(o); }
inline bool is_pynumeric (const py::handle &o) { return is_pyint(o) || is_pyfloat(o); }
inline bool is_pysequence (const py::handle &o) { return py::isinstance<py::sequence>(o); }
inline bool is_numpy_array_fc (const py::array &m) {
    return (m.flags() & py::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_) != 0;
}

//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////
std::string gretl4py_trim (std::string &s);

std::string gretl4py_to_lower (std::string s);

void NumPyArray_extract (const py::array &arr, NumPyArrayView &view);

py::object variant_to_pyobject (GretlTypes &&val);

template <typename T>
void get_object_from_dict (const py::dict &dict, const char *key, T &target);

GretlTypesPtr convert_GretlType_to_GretlTypes (GretlType t, void *gr_obj_ptr,
                                               const int n, int &err);

PythonArgInfo inspect_pyobject (py::handle obj);

template <typename PyWrapped, typename Item, typename ContainerType>
void move_to_python_container (PyWrapped &wrapped_obj, Item &&item,
                               const char *container, const std::string &key = "");

template <typename T>
void load_string_to_container (T &t, const std::string &src);

bool is_bool_kwarg (const py::handle &h, const std::string &key = "");

#endif // GRETL4PY_UTILS_H
