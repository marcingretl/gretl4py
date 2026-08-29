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

#include "gretl4py_bundle.h"
#include "gretl4py_utils.h"

namespace py = pybind11;

//
// GretlBundle mappings
//
void init_gretl4py_bundle_py11 (py::module_ &m)
{
    auto cls = py::class_<GretlBundle, GretlBundlePtr>(m, "Bundle", py::dynamic_attr());

    cls
    .def(py::init<>())                      // default constructor
    .def(py::init<const py::kwargs&>())     // on-the-fly initializer
    .def("__copy__",
         [](const GretlBundle &self) -> GretlBundlePtr
         {
             return std::make_shared<GretlBundle>(self);
         }, R"doc(Return a deep copy of the bundle.)doc")
    .def("__deepcopy__",
         [](const GretlBundle &self, py::dict) -> GretlBundlePtr
         {
             return std::make_shared<GretlBundle>(self);
         })
    .def("__repr__", &GretlBundle::repr_impl)
    .def("__getitem__", &GretlBundle::get_element<py::object>)
    .def("__setitem__", &GretlBundle::set_element<bool>)
    .def("__setitem__", &GretlBundle::set_element<int>)
    .def("__setitem__", &GretlBundle::set_element<double>)
    .def("__setitem__", &GretlBundle::set_element<std::string>)
    .def("__setitem__", &GretlBundle::set_element<py::array>)
    .def("__setitem__", &GretlBundle::set_element<GretlBundlePtr>)
    .def("__setitem__", &GretlBundle::set_element<GretlMatrixPtr>)
    .def("__delitem__", &GretlBundle::delete_element)
    .def("pop", &GretlBundle::pop, py::arg("key").none(false))
    .def("keys", &GretlBundle::keys)
    .def("print", &GretlBundle::print<void>, py::arg("tree").none(true) = false)
    .def("to_dict", &GretlBundle::to_dict);

    cls.attr("__module__") = "gretl";
}
