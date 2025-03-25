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

#include "gretl4py_data.h"

namespace py = pybind11;

void init_gretl4py_data_py11(py::module_ &m)
{
    py::class_<GretlDataset>(m, "GretlDataset")
        .def(py::init<const std::string>())
        .def(py::init<const py::dict &>())
        .def("get_name", &GretlDataset::get_name)
        .def("set_name", &GretlDataset::set_name)
        .def("get_id", &GretlDataset::get_id)
        .def("set_as_default", &GretlDataset::set_as_default)
        .def("to_dict", &GretlDataset::to_dict)     // verify ownership!!!
        .def("printdset", &GretlDataset::printdset)
        .def("ptr", &GretlDataset::ptr);
}
