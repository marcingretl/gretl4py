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

#include "gretl4py.h"
#include "gretl4py_utils.h"
#include "gretl4py_packages.h"

namespace py = pybind11;

//
// GretlPackage mappings
//
void init_gretl4py_packages_py11 (py::module_ &m)
{
    auto cls = py::class_<GretlPackage, GretlPackagePtr>(m, "Package", py::dynamic_attr());

    cls
    .def_property_readonly("version",
                               [](const GretlPackage &self)
                               {
                                   return self.get_version();
                               })
    .def_property_readonly("version_long",
                               [](const GretlPackage &self)
                               {
                                   return self.get_long_version();
                               })
    .def("__repr__", &GretlPackage::repr_impl)
    .def("get_functions", &GretlPackage::get_functions)
    .def("get_name", &GretlPackage::get_name)
    .def("show_functions", &GretlPackage::show_functions<false>,
         py::arg("name").none(true) = "")
    .def("show_signatures", &GretlPackage::show_functions<true>,
         py::arg("name").none(true) = "")
    .def("run_sample", &GretlPackage::run_sample)
    .def("_get_help", &GretlPackage::get_help)
    .def("_function_exec",
         [](GretlPackage &self, const std::string &name, const py::args &args, const py::kwargs &kwargs) -> py::object
         {
             GretlPrint printer = GretlPrint();
             auto dataset = GretlDataset::default_dataset();
             GretlTypesPtr val;
             int err = 0;
             auto res = self.function_exec(name, dataset, printer, args, kwargs);

             // we construct object to return
             val = convert_GretlType_to_GretlTypes(res.type, res.get(), res.n, err);
             if (err) handle_gretl_error(err);

             // we printout if needed
             const char *no_printing[] = { "quiet", "silent" };
             bool printout = true;

             for (const char *key : no_printing) {
                 if (kwargs.contains(key) && is_pybool(kwargs[key]) && kwargs[key].cast<bool>()) {
                     printout = false;
                     break;
                 }
             }
             if (printout) printer.print();

             return variant_to_pyobject(std::move(*val));
         }, py::arg("name").none(false), py::kw_only());

    cls.attr("__module__") = "gretl";
}
