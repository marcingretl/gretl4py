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

// signature from libgretl
const char *gretl_dotdir (void);

void init_gretl4py_data_py11(py::module_ &m)
{
    py::class_<GretlDataset>(m, "GretlDataset", py::dynamic_attr())
        .def(py::init<const std::string>())
        .def(py::init<const py::dict &>())
        .def("__copy__", [](const GretlDataset &src)
             {
                 const std::string dotdir = gretl_dotdir();
                 const std::string fname = dotdir + "gretl4py_tmp.gdt";

                 src.to_file(fname);
                 GretlDataset *dst = new GretlDataset(fname);
                 dst->source = src.source + " (copy of " + std::to_string(src.get_id()) + ")";

                 return dst;
             }, py::return_value_policy::take_ownership)
        .def_readonly("is_default", &GretlDataset::is_default)
        .def_readwrite("source", &GretlDataset::source)
        .def("__repr__", &GretlDataset::repr)
        .def("get_id", &GretlDataset::get_id)
        .def("set_as_default", &GretlDataset::set_as_default)
        .def("sample", &GretlDataset::sample)
        .def("setobs", &GretlDataset::setobs)
        .def("to_dict", &GretlDataset::to_dict,
             py::return_value_policy::take_ownership)
        .def("to_file", &GretlDataset::to_file)
        .def("new_series", &GretlDataset::new_series)
        .def("print", &GretlDataset::print)
        .def("varnames", &GretlDataset::varnames,
             py::return_value_policy::take_ownership)
        .def("linked_models_list", &GretlDataset::linked_models_list)
        .def("get_accessor", &GretlDataset::get_accessor)
        .def("test",
             [](GretlDataset &self,
                const std::string &type,
                const py::kwargs &kwargs) -> py::dict
             {
                 std::string vars;
                 py::dict clean_kwargs;

                 if (type == "coint") {
                     // we check if both 'y' and 'vars' exist
                     if (kwargs.contains("y") && kwargs.contains("vars")) {
                         vars = std::string(py::str(kwargs["y"])) + " " + std::string(py::str(kwargs["vars"]));
                     } else {
                         throw std::invalid_argument("Both 'y' and 'vars' are required");
                     }
                     for (auto item : kwargs) {
                         std::string key = std::string(py::str(item.first));
                         if (key != "y" && key != "vars") {
                             clean_kwargs[item.first] = item.second;
                         }
                     }
                 } else {
                     if (kwargs.contains("vars")) {
                         vars = std::string(py::str(kwargs["vars"]));
                     } else {
                         throw std::invalid_argument("'vars' are required");
                     }
                     for (auto item : kwargs) {
                         std::string key = std::string(py::str(item.first));
                         if (key != "vars") {
                             clean_kwargs[item.first] = item.second;
                         }
                     }
                 }

                 return self.test(type, vars, clean_kwargs);
             },
             py::arg("type"), py::kw_only(),
             py::return_value_policy::take_ownership);
}
