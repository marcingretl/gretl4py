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
#define is_pybool(o) (py::isinstance<py::bool_>(o))
#define is_pyint(o) (py::isinstance<py::int_>(o))
#define is_pyfloat(o) (py::isinstance<py::float_>(o))
#define is_pystr(o) (py::isinstance<py::str>(o))
#define is_pydict(o) (py::isinstance<py::dict>(o))
#define is_pytuple(o) (py::isinstance<py::tuple>(o))
#define is_pylist(o) (py::isinstance<py::list>(o))
#define is_pynumarray(o) (py::isinstance<py::array>(o))
#define is_pynumarrayd(o) (py::isinstance<py::array_t<double>>(o))
#define is_pynumeric(o) (is_pyint(o) || is_pyfloat(o))
#define is_pysequence(o) (py::isinstance<py::sequence>(o))
#define is_pyGretlBundle(o) (py::isinstance<GretlBundle>(o))
#define is_NumPyArrayFC(m) ((m.flags() & py::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_))

#define SET_OPT(opt, flag) opt = static_cast<gretlopt>((opt) | (flag))

//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////
std::string gretl4py_trim (std::string &s);

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

//
// This is general purpose py::object/py::handle to C++ types mapper
//
class MapPyToCpp
{
private:
    std::string as_str_;

    template <typename T>
    void convert_ (const T *pyobj);
public:
    int err = 0;

    MapPyToCpp (const std::optional<py::object> &pyobject);
    MapPyToCpp (const py::handle &pyhandle);

    inline std::string to_string (void) const { return as_str_; }

    inline std::string_view to_string_view (void) const { return static_cast<std::string_view>(as_str_); }
};

#endif // GRETL4PY_UTILS_H
