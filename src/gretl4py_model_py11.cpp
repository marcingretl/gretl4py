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

#include "gretl4py_model.h"

namespace py = pybind11;

void init_gretl4py_model_py11(py::module_ &m)
{
    py::class_<GretlModel, std::unique_ptr<GretlModel>>(m, "GretlModel", py::dynamic_attr())
        .def("get_id", &GretlModel::get_id)
        .def("unlink_from_dataset", &GretlModel::unlink_from_dataset)
        .def("fit", &GretlModel::fit)
        .def("forecast", &GretlModel::forecast)
        .def("restrict", &GretlModel::restrict)
        .def("test",
             py::overload_cast<const std::string&, const py::kwargs&>(&GretlModel::test),
             py::arg("type").none(false),
             py::kw_only())
        .def("test",
             py::overload_cast<const std::string&, const std::string&, const py::kwargs&>(&GretlModel::test),
             py::arg("type").none(false),
             py::arg("vars").none(false),
             py::kw_only());

    py::class_<GretlModel_NSE, GretlModel, std::unique_ptr<GretlModel_NSE>>(m, "GretlModel_NSE", py::dynamic_attr())
        .def("forecast", &GretlModel_NSE::forecast)
        .def("restrict", &GretlModel_NSE::restrict)
        .def("test",
             py::overload_cast<const std::string&, const py::kwargs&>(&GretlModel_NSE::test),
             py::arg("type").none(false),
             py::kw_only())
        .def("test",
             py::overload_cast<const std::string&, const std::string&, const py::kwargs&>(&GretlModel_NSE::test),
             py::arg("type").none(false),
             py::arg("vars").none(false),
             py::kw_only());

    py::class_<GretlModel_NSE_arima, GretlModel_NSE, std::unique_ptr<GretlModel_NSE_arima>>(m, "GretlModel_NSE_arima", py::dynamic_attr())
        .def(py::init<std::shared_ptr<GretlDataset>, const std::string&, const py::tuple&>())
        .def("fit", &GretlModel_NSE_arima::fit)
        .def("fit2",
             [](GretlModel_NSE_arima &self) -> GretlModel_NSE_arima&
             {
                 self.fit2();

                 return self;
             });

    m.def("arima2",
          [](const std::string &y, const py::tuple &order, const py::kwargs &kwargs) -> std::unique_ptr<GretlModel>
          {
              auto model = std::make_unique<GretlModel_NSE_arima>(DSET, y, order, kwargs);

              //py::object self = py::cast(model.get());
              //py::setattr(self, "res", res);

              return std::unique_ptr<GretlModel>(std::move(model));
          },
          py::arg("y").none(false), py::arg("order").none(false), py::kw_only());
}
