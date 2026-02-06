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
#include "gretl_calls.h"
#include "gretl4py_utils.h"
#include "gretl4py_packages.h"

namespace py = pybind11;

// signature from libgretl
const char *gretl_workdir (void);

enum HeadTail { HEAD, TAIL };       // for head/tail printing

template <HeadTail mode>
static void head_tail_print (GretlDataset &gd, int nobs)
{
    //
    // here we perform ::head() and ::tail()
    // Note: this function is actually a wrapper to ::print()
    // thus should be used only for head()/tail() mapping
    //
    py::kwargs opts;

    // we setup printing parameters
    if constexpr (mode == HEAD) {
        opts["head"] = nobs;
    } else if constexpr (mode == TAIL) {
        opts["tail"] = nobs;
    } else {
        static_assert(always_false<HeadTail>, "head_tail_print: unknown mode");
    }

    gd.print("", opts);
}

// GretlDataset mappings
void init_gretl4py_dataset_py11(py::module_ &m)
{
    py::class_<GretlDataset, GretlDatasetPtr>(m, "Dataset", py::dynamic_attr())
        .def_readonly("is_default", &GretlDataset::is_default)
        .def_readonly("source", &GretlDataset::source)
        .def_property_readonly("_is_panel",
                               [](const GretlDataset &self)
                               {
                                   return self.is_panel();
                               })
        .def_property_readonly("n",
                               [](const GretlDataset &self)
                               {
                                   return self.dset->n;
                               })
        .def_property_readonly("k",
                               [](const GretlDataset &self)
                               {
                                   return self.dset->v;
                               })
        .def_property_readonly("obs_labels",
                               [](const GretlDataset &self)
                               {
                                   return self.get_obs_labels();
                               })
        .def_property_readonly("id",
                               [](const GretlDataset &self)
                               {
                                   return self.get_id();
                               })
        .def_property_readonly("linked_models_list",
                               [](const GretlDataset &self)
                               {
                                   return self.linked_models_list();
                               })
        .def("__copy__",
             [](const GretlDataset &self)
             {
                 return GretlDataset::clone(self);
             },
             R"doc(Return a deep copy of the dataset.)doc")
        .def("__deepcopy__", [](const GretlDataset &self, py::dict) { return GretlDataset::clone(self); })
        .def("__repr__", &GretlDataset::repr)
        .def("__getitem__", &GretlDataset::getitem_impl<py::slice>)
        .def("__getitem__", &GretlDataset::getitem_impl<py::list>)
        .def("__getitem__", &GretlDataset::getitem_impl<std::string>)
        .def("clone", [](const GretlDataset &self) { return GretlDataset::clone(self); })
        .def("head", &head_tail_print<HEAD>, py::arg("nobs") = 5, py::kw_only())
        .def("tail", &head_tail_print<TAIL>, py::arg("nobs") = 5, py::kw_only())
        .def("set_as_default", &GretlDataset::set_as_default)
        .def("sample",
             [](GretlDataset &self, const std::string &startobs,
                const std::string &endobs, const py::kwargs &kwargs)
             {
                 // direct call to GretlDataset::smpl()
                 self.smpl(startobs, endobs, kwargs);
             },
             py::arg("startobs").none(true) = "", py::arg("endobs").none(true) = "", py::kw_only())
        .def("sample",
             [](GretlDataset &self, const std::string &startobs, const py::kwargs &kwargs)
             {
                 self.smpl(startobs, "", kwargs);
             },
             py::arg("startobs").none(false), py::kw_only())
        .def("sample",
             [](GretlDataset &self, int startobs, int endobs, const py::kwargs &kwargs)
             {
                 std::string startobs_str = startobs == NADBL ? "" : std::to_string(startobs);
                 std::string endobs_str = endobs == NADBL ? "" : std::to_string(endobs);

                 self.smpl(startobs_str, endobs_str, kwargs);
             },
             py::arg("startobs").none(true) = NADBL,
             py::arg("endobs").none(true) = NADBL, py::kw_only())
        .def("sample",
             [](GretlDataset &self, int startobs, const py::kwargs &kwargs)
             {
                 self.smpl(std::to_string(startobs), "", kwargs);
             },
             py::arg("startobs").none(false), py::kw_only())
        .def("setobs",
             [](GretlDataset &self, std::string &periodicity, const std::string &startobs, const py::kwargs &kwargs)
             {
                 self.setobs(periodicity, startobs, kwargs);
             },
             py::arg("periodicity").none(false),
             py::arg("startobs").none(false), py::kw_only())
        .def("setobs",
             [](GretlDataset &self, int periodicity, const std::string &startobs, const py::kwargs &kwargs)
             {
                 self.setobs(std::to_string(periodicity), startobs, kwargs);
             },
             py::arg("periodicity").none(false), py::arg("startobs").none(false), py::kw_only())
        .def("setobs",
             [](GretlDataset &self, int periodicity, int startobs, const py::kwargs &kwargs)
             {
                 self.setobs(std::to_string(periodicity), std::to_string(startobs), kwargs);
             },
             py::arg("periodicity").none(false),
             py::arg("startobs").none(false), py::kw_only())
        .def("to_dict", &GretlDataset::to_dict)
        .def("to_file", &GretlDataset::to_file,
             py::arg("filename").none(false),
             py::arg("varlist").none(true) = "", py::kw_only())
        .def("add_series", py::overload_cast<const py::array&, const std::string&>(&GretlDataset::add_series),
             py::arg("data").none(false), py::arg("name").none(false))
        .def("new_series", py::overload_cast<const std::string&>(&GretlDataset::new_series),
             py::arg("formula").none(false))
        .def("new_series", py::overload_cast<const std::string&, const std::string&>(&GretlDataset::new_series),
             py::arg("name").none(false), py::arg("value").none(false))
        .def("rename", &GretlDataset::rename,
             py::arg("series").none(false), py::arg("newname").none(false), py::kw_only())
        .def("rename",
             [](GretlDataset &self, const std::string &series,
                const std::string &newname, const py::kwargs &kwargs)
             {
                 self.rename(self.get_series_id(series, false), newname, kwargs);
             },
             py::arg("series").none(false), py::arg("newname").none(false), py::kw_only())
        .def("rename",
             [](GretlDataset &self, const py::kwargs &kwargs)
             {
                 self.rename(-1, "", kwargs);
             })
        .def("new_list", py::overload_cast<const std::string&>(&GretlDataset::new_list),
             py::arg("formula").none(false))
        .def("new_list", py::overload_cast<const std::string&, const std::string&>(&GretlDataset::new_list),
             py::arg("name").none(false), py::arg("value").none(false))
        .def("dummify", &GretlDataset::transforms<DUMMIFY>,
             py::arg("varlist").none(false), py::kw_only())
        .def("square", &GretlDataset::transforms<SQUARE>,
             py::arg("varlist").none(false), py::kw_only())
        .def("discrete", &GretlDataset::transforms<DISCRETE>,
             py::arg("varlist").none(false), py::kw_only())
        .def("stdize", &GretlDataset::transforms<STDIZE>,
             py::arg("varlist").none(false), py::kw_only())
        .def("diff", &GretlDataset::transforms_no_opts<DIFF>,
             py::arg("varlist").none(false))
        .def("ldiff", &GretlDataset::transforms_no_opts<LDIFF>,
             py::arg("varlist").none(false))
        .def("sdiff", &GretlDataset::transforms_no_opts<SDIFF>,
             py::arg("varlist").none(false))
        .def("logs", &GretlDataset::transforms_no_opts<LOGS>,
             py::arg("varlist").none(false))
        .def("orthdev", &GretlDataset::transforms_no_opts<ORTHDEV>,
             py::arg("varlist").none(false))
        .def("lags", &GretlDataset::lags,
             py::arg("varlist").none(false), py::arg("order").none(true) = 0)
        .def("print", &GretlDataset::print,
             py::arg("varlist").none(true) = "", py::kw_only())
        .def("varnames", &GretlDataset::varnames)
        .def("get_series", &GretlDataset::get_series,
             py::arg("series").none(false))
        .def("hpfilt", &GretlDataset::hpfilt,
             py::arg("y").none(false), py::arg("hp_lambda").none(true) = NADBL,
             py::arg("trend").none(true) = false, py::arg("one_sided").none(true) = false)
        .def("hpfilt",
             [](GretlDataset &self, const std::string &y, double hp_lambda, bool one_sided)
             {
                 return self.hpfilt(y, hp_lambda, false, one_sided);
             },
             py::arg("y").none(false), py::arg("hp_lambda").none(true) = NADBL,
             py::arg("one_sided").none(true) = false)
        .def("bwfilt", &GretlDataset::bwfilt,
             py::arg("y").none(false), py::arg("n").none(false),
             py::arg("omega").none(false))
        .def("bkfilt", &GretlDataset::bkfilt,
             py::arg("y").none(false), py::arg("f1").none(true) = 0,
             py::arg("f2").none(true) = 0, py::arg("k").none(true) = 0)
        .def("get_accessor", &GretlDataset::get_accessor,
             py::arg("name").none(false))
        .def("corr", &GretlDataset::corr,
             py::arg("varlist").none(true) = "", py::kw_only())
        .def("cov", &GretlDataset::cov, py::arg("vars").none(true) = "")
        .def("hurst", &GretlDataset::hurst, py::arg("series").none(false))
        .def("xcorrgm",
             [](GretlDataset &self, const std::string &varlist, int order,
                const py::kwargs &kwargs)
             {
                 return self.xcorrgm(varlist, order, kwargs);
             },
             py::arg("varlist").none(false), py::arg("order").none(true) = 0)
        .def("xcorrgm",
             [](GretlDataset &self, const std::string &series1,
                const std::string &series2, int order, const py::kwargs &kwargs)
             {
                 return self.xcorrgm(series1 + " " + series2, order, kwargs);
             },
             py::arg("series1").none(false), py::arg("series2").none(false),
             py::arg("order").none(true) = 0)
        .def("pca", &GretlDataset::pca,
             py::arg("varlist").none(true) = "", py::kw_only())
        .def("test",
             [](GretlDataset &self, const std::string &type, const py::kwargs &kwargs)
             {
                 //
                 // here we do some pre-computations for certain tests
                 //
                 std::string y_val, vars_val, varlist;
                 py::kwargs clean_kwargs;

                 for (const auto &item : kwargs) {
                     std::string key = std::string(py::str(item.first));
                     if (key == "y") {
                         y_val = std::string(py::str(item.second));
                     } else if (key == "varlist") {
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

                 // some special cases
                 if (type == "coint") {
                     if (y_val.empty() || vars_val.empty()) throw std::invalid_argument("Both 'y' and 'varlist' are required for 'coint'");
                     varlist = y_val + " " + vars_val;
                 } else if (type == "corrgm") {
                     if (y_val.empty()) throw std::invalid_argument("'y' is required");
                     varlist = y_val;
                 } else {
                     if (vars_val.empty()) throw std::invalid_argument("'varlist' is required");
                     varlist = vars_val;
                 }

                 // we do the job
                 self.test(type, varlist, clean_kwargs);
             },
             py::arg("type"), py::kw_only());
}

// GretlPackage mappings
void init_gretl4py_package_py11(py::module_ &m)
{
    py::class_<GretlPackage, GretlPackagePtr>(m, "Package", py::dynamic_attr())
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
    .def("get_name", &GretlPackage::get_name)
    .def("show_functions", &GretlPackage::show_functions,
         py::arg("name").none(true) = "")
    .def("set_main", &GretlPackage::set_main,
         py::arg("name").none(false))
    .def("get_functions", &GretlPackage::get_functions)
    .def("_get_help", &GretlPackage::get_help)
    .def("_function_exec",
         [](GretlPackage &self, const std::string &name,
            const py::args &args, const py::kwargs &kwargs) -> py::object
         {
             GretlPrint GP = GretlPrint();
             auto dataset = GretlDataset::default_dataset();
             GretlTypesPtr val;
             int err = 0;

             auto res = self.function_exec(name, dataset, GP,
                                           args, kwargs);

             // we construct object to return
             val = convert_GretlType_to_GretlTypes(res.type, res.get(),
                                                   res.n, err);
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
             if (printout) GP.print();

             return variant_to_pyobject(std::move(*val));
         },
         py::arg("name").none(false), py::kw_only());
}

//GretlBundle mappings
void init_gretl4py_bundle_py11(py::module_ &m)
{
    py::class_<GretlBundle, GretlBundlePtr>(m, "Bundle", py::dynamic_attr())
    .def(py::init<>())                      // default constructor
    .def(py::init<const py::kwargs&>())     // on-the-fly initializer
    .def("__repr__", &GretlBundle::repr_impl)
    .def("__getitem__", &GretlBundle::get_element<py::object>)
    .def("__setitem__", &GretlBundle::set_element<bool>)
    .def("__setitem__", &GretlBundle::set_element<int>)
    .def("__setitem__", &GretlBundle::set_element<double>)
    .def("__setitem__", &GretlBundle::set_element<std::string>)
    .def("__setitem__", &GretlBundle::set_element<py::array>)
    .def("__setitem__", &GretlBundle::set_element<GretlBundlePtr>)
    .def("__delitem__", &GretlBundle::delete_element)
    .def("pop", &GretlBundle::pop, py::arg("key").none(false))
    .def("keys", &GretlBundle::keys)
    .def("print", &GretlBundle::print<void>, py::arg("tree").none(true) = false)
    .def("to_dict", &GretlBundle::to_dict);
}
