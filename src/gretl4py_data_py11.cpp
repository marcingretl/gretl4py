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
const char *gretl_workdir (void);

void init_gretl4py_data_py11(py::module_ &m)
{
    py::class_<GretlDataset, std::shared_ptr<GretlDataset>>(m, "Dataset", py::dynamic_attr())
        .def(py::init([](const std::string &fname) { return std::make_shared<GretlDataset>(fname); }))
        .def(py::init([](const py::dict &pydict) { return std::make_shared<GretlDataset>(pydict); }))
        .def("__copy__",
             [](const GretlDataset &src)
             {
                 const std::string workdir = gretl_workdir();
                 const std::string fname = workdir + "gretl4py_tmp.gdt";

                 src.to_file(fname);
                 auto dst = std::make_shared<GretlDataset>(fname);
                 dst->source = src.source + " (copy of " + std::to_string(src.get_id()) + ")";

                 return dst;
             },
             py::return_value_policy::take_ownership)
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
        .def("new_list", &GretlDataset::new_list)
        .def("print", &GretlDataset::print)
        .def("varnames", &GretlDataset::varnames,
             py::return_value_policy::take_ownership)
        .def("get_series", &GretlDataset::get_series,
             py::arg("series").none(false),
             py::return_value_policy::take_ownership)
        .def("linked_models_list", &GretlDataset::linked_models_list)
        .def("hpfilt", &GretlDataset::hpfilt,
             py::arg("series").none(false), py::arg("hp_lambda").none(true) = NADBL,
             py::arg("trend").none(true) = false, py::arg("one_sided").none(true) = false,
             py::return_value_policy::take_ownership)
        .def("get_accessor", // this is due to: error: static assertion failed: Holder classes are only supported for custom types
             [](GretlDataset &self, const std::string &accessor) -> GretlTypes*
             {
                 return self.get_accessor(accessor).release();
             },
             py::return_value_policy::take_ownership)
        .def("test",
             [](GretlDataset &self, const std::string &type, const py::kwargs &kwargs)
             {
                 //
                 // here we do some things for certain tests
                 //
                 std::string y_val, vars_val, vars;
                 py::kwargs clean_kwargs;

                 for (auto item : kwargs) {
                     std::string key = std::string(py::str(item.first));
                     if (key == "y") {
                         y_val = std::string(py::str(item.second));
                     } else if (key == "vars") {
                         vars_val = std::string(py::str(item.second));
                     } else if (key == "order" && type == "levinlin" && is_pynumarray(item.second)) {
                         // order was passed as matrix: we cannot pass it to method as order (which is <int>)
                         clean_kwargs[py::cast("order_m")] = item.second;
                     } else if (key == "order" && type == "levinlin" && is_pylist(item.second)) {
                         // order was passed as list: we cannot pass it to method as order (which is <int>)
                         clean_kwargs[py::cast("order_m")] = py::cast<py::array>(item.second);
                     } else {
                         clean_kwargs[item.first] = item.second;
                     }
                 }
                 if (type == "coint") {
                     if (y_val.empty() || vars_val.empty()) {
                         throw std::invalid_argument("Both 'y' and 'vars' are required for 'coint'");
                     }
                     vars = y_val + " " + vars_val;
                 } else {
                     if (vars_val.empty()) {
                         throw std::invalid_argument("'vars' is required");
                     }
                     vars = vars_val;
                 }

                 self.test(type, vars, clean_kwargs);
             },
             py::arg("type"), py::kw_only(),
             py::return_value_policy::take_ownership);
}
