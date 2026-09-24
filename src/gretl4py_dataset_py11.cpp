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

#include "gretl4py_dataset.h"
#include "gretl4py_utils.h"
#include "docstrings/gretl4py_dataset_docstr.h"

namespace py = pybind11;

// signature from libgretl
const char *gretl_workdir (void);

enum HeadTail { HEAD, TAIL };       // for head/tail printing

namespace {
template <HeadTail mode>
void head_tail_print (GretlDataset &dataset, int nobs)
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

    dataset.print("", opts);
}
}

//
// GretlDataset mappings
//
void init_gretl4py_dataset_py11 (py::module_ &m)
{
    auto cls = py::class_<GretlDataset, GretlDatasetPtr>(m, "Dataset", py::dynamic_attr());

    cls
    .def_readonly("is_default", &GretlDataset::is_default)
    .def_readonly("source", &GretlDataset::source)
    .def_property_readonly("is_panel",
                           [](const GretlDataset &self)
                           {
                               return self.is_panel();
                           })
    .def_property_readonly("is_time_series",
                           [](const GretlDataset &self)
                           {
                               return self.is_time_series();
                           })
    .def_property_readonly("is_cross_section",
                           [](const GretlDataset &self)
                           {
                               return self.is_cross_section();
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
    .def_property_readonly("varnames",
                           [](const GretlDataset &self)
                           {
                               return self.varnames();
                           })
    .def("__copy__",
         [](const GretlDataset &self) -> GretlDatasetPtr
         {
             return GretlDataset::clone(self);
         }, doc::Dataset::clone)
    .def("__deepcopy__",
         [](const GretlDataset &self, py::dict) -> GretlDatasetPtr
         {
             return GretlDataset::clone(self);
         }, doc::Dataset::clone)
    .def("__repr__", &GretlDataset::repr)
    .def("__getitem__", py::overload_cast<const py::slice&>(&GretlDataset::getitem_impl))
    .def("__getitem__", py::overload_cast<const std::string&>(&GretlDataset::getitem_impl))
    .def("__getitem__", py::overload_cast<const py::list&>(&GretlDataset::getitem_impl))
    .def("_test",
         [](GretlDataset &self, const std::string &type, const py::kwargs &kwargs) -> GretlBundlePtr
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
                     if (is_pystr(item.second)) {
                         vars_val = item.second.cast<std::string>();
                     } else if (is_pylist(item.second)) {
                         auto list = item.second.cast<py::list>();
                         auto varnames = self.varnames();

                         if (py::len(list) == 0) {
                             vars_val = "";
                         } else {
                             for (const auto &el : list) {
                                 if (is_pystr(el)) {
                                     vars_val += el.cast<std::string>();
                                 } else if (is_pyint(el)) {
                                     vars_val += varnames[el].cast<std::string>();
                                 } else {
                                     throw std::invalid_argument("list elements must be integers or strings");
                                 }
                                 vars_val += " ";
                             }
                             gretl4py_trim(vars_val);
                         }
                     } else {
                         throw std::invalid_argument("varlist: unsupported type");
                     }
                 } else if (key == "order" && type == "levinlin" && is_pynumarray(item.second)) {
                     // order was passed as matrix: we cannot pass it to method as order (which is <int>)
                     clean_kwargs[py::cast("order_m")] = item.second;
                 } else if (key == "order" && type == "levinlin" && is_pygretlmatrix(item.second)) {
                     // order was passed as GretlMatrix: we cannot pass it to method as order (which is <int>)
                     clean_kwargs[py::cast("order_m")] = py::cast<py::array>(py::cast<GretlMatrixPtr>(item.second)->to_numpy());
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
             return self.test(type, varlist, clean_kwargs);
         }, py::arg("type"), py::kw_only())
    .def("clone", [](const GretlDataset &self) { return GretlDataset::clone(self); }, doc::Dataset::clone)
    .def("head", &head_tail_print<HEAD>, py::arg("nobs") = 5, py::kw_only())
    .def("tail", &head_tail_print<TAIL>, py::arg("nobs") = 5, py::kw_only())
    .def("set_as_default", &GretlDataset::set_as_default)
    .def("summary",
         [](const GretlDataset &self) -> void
         {
             self.summary();
         })
    .def("sample",
         [](GretlDataset &self, const std::string &startobs, const std::string &endobs, const py::kwargs &kwargs) -> void
         {
             self.smpl(startobs, endobs, kwargs);
         }, py::arg("startobs").none(true) = "", py::arg("endobs").none(true) = "", py::kw_only())
    .def("sample",
         [](GretlDataset &self, const std::string &startobs, const py::kwargs &kwargs) -> void
         {
             self.smpl(startobs, "", kwargs);
         }, py::arg("startobs").none(false), py::kw_only())
    .def("sample",
         [](GretlDataset &self, int startobs, int endobs, const py::kwargs &kwargs) -> void
         {
             std::string startobs_str = startobs == NADBL ? "" : std::to_string(startobs);
             std::string endobs_str = endobs == NADBL ? "" : std::to_string(endobs);

             self.smpl(startobs_str, endobs_str, kwargs);
         }, py::arg("startobs").none(true) = NADBL, py::arg("endobs").none(true) = NADBL, py::kw_only())
    .def("sample",
         [](GretlDataset &self, int startobs, const py::kwargs &kwargs) -> void
         {
             self.smpl(std::to_string(startobs), "", kwargs);
         }, py::arg("startobs").none(false), py::kw_only(), doc::Dataset::sample)
    .def("setobs",
         [](GretlDataset &self, std::string &periodicity, const std::string &startobs, const py::kwargs &kwargs) -> void
         {
             self.setobs(periodicity, startobs, kwargs);
         }, py::arg("periodicity").none(false), py::arg("startobs").none(false), py::kw_only())
    .def("setobs",
         [](GretlDataset &self, int periodicity, const std::string &startobs, const py::kwargs &kwargs) -> void
         {
             self.setobs(std::to_string(periodicity), startobs, kwargs);
         }, py::arg("periodicity").none(false), py::arg("startobs").none(false), py::kw_only())
    .def("setobs",
         [](GretlDataset &self, int periodicity, int startobs, const py::kwargs &kwargs) -> void
         {
             self.setobs(std::to_string(periodicity), std::to_string(startobs), kwargs);
         }, py::arg("periodicity").none(false), py::arg("startobs").none(false), py::kw_only())
    .def("addobs",
         [](GretlDataset &self, int n, const py::kwargs &kwargs) -> void
         {
             self.do_modify_dataset<std::nullptr_t>(DS_ADDOBS, nullptr, std::to_string(n), kwargs);
         }, py::arg("n").none(false), py::kw_only())
    .def("insobs",
         [](GretlDataset &self, int n) -> void
         {
             self.do_modify_dataset<std::nullptr_t>(DS_INSOBS, nullptr, std::to_string(n), py::dict());
         }, py::arg("n").none(false))
    .def("compact",
         [](GretlDataset &self, py::object freq, const std::string &method, const py::kwargs &kwargs) -> void
         {
             std::string formula;

             if (is_pyint(freq)) {
                 formula = std::to_string(freq.cast<int>());
             } else if (is_pystr(freq)) {
                 formula = freq.cast<std::string>();
             } else {
                 std::string msg = "must be int or string, is: ";
                 msg.append(get_pyobject_type(freq));
                 handle_gretl_error(E_INVARG, "start", msg);
             }

             if (!method.empty()) formula.append(" ").append(method);

             self.do_modify_dataset<std::nullptr_t>(DS_COMPACT, nullptr, formula, kwargs);
         }, py::arg("freq").none(false), py::arg("method").none(true) = "", py::kw_only(), doc::Dataset::compact)
    .def("expand",
         [](GretlDataset &self, int freq) -> void
         {
             self.do_modify_dataset<std::nullptr_t>(DS_EXPAND, nullptr, std::to_string(freq), py::dict());
         }, py::arg("freq").none(true) = -999, doc::Dataset::expand)
    .def("transpose",
         [](GretlDataset &self) -> void
         {
             self.do_modify_dataset<std::nullptr_t>(DS_TRANSPOSE, nullptr, "", py::dict());
         }, doc::Dataset::transpose)
    .def("sortby",
         [](GretlDataset &self, const std::string &varlist) -> void
         {
             self.do_modify_dataset<std::string>(DS_SORTBY, varlist, "", py::dict());
         }, py::arg("varlist").none(false))
    .def("sortby",
         [](GretlDataset &self, const py::list &varlist) -> void
         {
             self.do_modify_dataset<py::list>(DS_SORTBY, varlist, "", py::dict());
         }, py::arg("varlist").none(false), doc::Dataset::sortby)
    .def("dsortby",
         [](GretlDataset &self, const std::string &varlist) -> void
         {
             self.do_modify_dataset<std::string>(DS_DSORTBY, varlist, "", py::dict());
         }, py::arg("varlist").none(false))
    .def("dsortby",
         [](GretlDataset &self, const py::list &varlist) -> void
         {
             self.do_modify_dataset<py::list>(DS_DSORTBY, varlist, "", py::dict());
         }, py::arg("varlist").none(false), doc::Dataset::dsortby)
    .def("resample",
         [](GretlDataset &self, int n) -> void
         {
             self.do_modify_dataset<std::nullptr_t>(DS_RESAMPLE, nullptr, std::to_string(n), py::dict());
         }, py::arg("n").none(false), doc::Dataset::resample)
    .def("pad_daily",
         [](GretlDataset &self, int days) -> void
         {
             self.do_modify_dataset<std::nullptr_t>(DS_PAD_DAILY, nullptr, std::to_string(days), py::dict());
         }, py::arg("days").none(false), doc::Dataset::pad_daily)
    .def("unpad_daily",
         [](GretlDataset &self) -> void
         {
             self.do_modify_dataset<std::nullptr_t>(DS_UNPAD_DAILY, nullptr, "", py::dict());
         }, doc::Dataset::unpad_daily)
    .def("tdisagg", &GretlDataset::tdisagg,
         py::arg("Y").none(false), py::arg("s").none(false), py::kw_only(), doc::Dataset::tdisagg)
    .def("to_dict", &GretlDataset::to_dict, doc::Dataset::to_dict)
    .def("to_file", py::overload_cast<const std::string&, const std::string&, const py::kwargs&>(&GretlDataset::to_file<std::string>),
         py::arg("filename").none(false), py::arg("varlist").none(true) = "", py::kw_only())
    .def("to_file", py::overload_cast<const std::string&, const py::list&, const py::kwargs&>(&GretlDataset::to_file<py::list>),
         py::arg("filename").none(false), py::arg("varlist").none(true) = py::list(), py::kw_only())
    .def("to_matrix", &GretlDataset::to_matrix<std::string>,
        py::arg("list").none(true) = py::list(), py::arg("startobs").none(true) = 0,
        py::arg("endobs").none(true) = 0, py::arg("missing").none(true) = "ok")
    .def("to_matrix", &GretlDataset::to_matrix<py::list>,
         py::arg("list").none(true) = py::list(), py::arg("startobs").none(true) = 0,
         py::arg("endobs").none(true) = 0, py::arg("missing").none(true) = "ok")
    .def("add_series", py::overload_cast<const py::array&, const std::string&>(&GretlDataset::add_series),
         py::arg("data").none(false), py::arg("name").none(false))
    .def("add_series", py::overload_cast<const GretlMatrix&, const std::string&>(&GretlDataset::add_series),
         py::arg("data").none(false), py::arg("name").none(false))
    .def("new_series", py::overload_cast<const std::string&>(&GretlDataset::new_series),
         py::arg("formula").none(false))
    .def("new_series", py::overload_cast<const std::string&, const std::string&>(&GretlDataset::new_series),
         py::arg("name").none(false), py::arg("value").none(false))
    .def("rename", &GretlDataset::rename,
         py::arg("series").none(false), py::arg("newname").none(false), py::kw_only())
    .def("rename",
         [](GretlDataset &self, const std::string &series, const std::string &newname, const py::kwargs &kwargs)
         {
             self.rename(self.get_series_id(series, false), newname, kwargs);
         }, py::arg("series").none(false), py::arg("newname").none(false), py::kw_only())
    .def("rename",
         [](GretlDataset &self, const py::kwargs &kwargs) -> void
         {
             self.rename(-1, "", kwargs);
         })
    .def("new_list", py::overload_cast<const std::string&>(&GretlDataset::new_list),
         py::arg("formula").none(false))
    .def("new_list", py::overload_cast<const std::string&, const std::string&>(&GretlDataset::new_list),
         py::arg("name").none(false), py::arg("value").none(false))
    .def("dummify", &GretlDataset::transforms<std::string, DUMMIFY>,
         py::arg("varlist").none(false), py::kw_only())
    .def("square", &GretlDataset::transforms<std::string, SQUARE>,
         py::arg("varlist").none(false), py::kw_only())
    .def("discrete", &GretlDataset::transforms<std::string, DISCRETE>,
         py::arg("varlist").none(false), py::kw_only())
    .def("stdize", &GretlDataset::transforms<std::string, STDIZE>,
         py::arg("varlist").none(false), py::kw_only())
    .def("dummify", &GretlDataset::transforms<py::list, DUMMIFY>,
         py::arg("varlist").none(false), py::kw_only())
    .def("square", &GretlDataset::transforms<py::list, SQUARE>,
         py::arg("varlist").none(false), py::kw_only())
    .def("discrete", &GretlDataset::transforms<py::list, DISCRETE>,
         py::arg("varlist").none(false), py::kw_only())
    .def("stdize", &GretlDataset::transforms<py::list, STDIZE>,
         py::arg("varlist").none(false), py::kw_only())
    .def("diff", &GretlDataset::transforms_no_opts<std::string, DIFF>,
         py::arg("varlist").none(false))
    .def("ldiff", &GretlDataset::transforms_no_opts<std::string, LDIFF>,
         py::arg("varlist").none(false))
    .def("sdiff", &GretlDataset::transforms_no_opts<std::string, SDIFF>,
         py::arg("varlist").none(false))
    .def("logs", &GretlDataset::transforms_no_opts<std::string, LOGS>,
         py::arg("varlist").none(false))
    .def("orthdev", &GretlDataset::transforms_no_opts<std::string, ORTHDEV>,
         py::arg("varlist").none(false))
    .def("diff", &GretlDataset::transforms_no_opts<py::list, DIFF>,
         py::arg("varlist").none(false))
    .def("ldiff", &GretlDataset::transforms_no_opts<py::list, LDIFF>,
         py::arg("varlist").none(false))
    .def("sdiff", &GretlDataset::transforms_no_opts<py::list, SDIFF>,
         py::arg("varlist").none(false))
    .def("logs", &GretlDataset::transforms_no_opts<py::list, LOGS>,
         py::arg("varlist").none(false))
    .def("orthdev", &GretlDataset::transforms_no_opts<py::list, ORTHDEV>,
         py::arg("varlist").none(false))
    .def("lags", &GretlDataset::lags<std::string>,
         py::arg("varlist").none(false), py::arg("order").none(true) = 0)
    .def("lags", &GretlDataset::lags<py::list>,
         py::arg("varlist").none(false), py::arg("order").none(true) = 0)
    .def("print", py::overload_cast<const std::string&, const py::kwargs&>(&GretlDataset::print),
         py::arg("varlist").none(true) = "", py::kw_only())
    .def("print", py::overload_cast<const py::list&, const py::kwargs&>(&GretlDataset::print),
         py::arg("varlist").none(true) = py::list(), py::kw_only())
    .def("get_series", &GretlDataset::get_series,
         py::arg("series").none(false))
    .def("hpfilt", &GretlDataset::hpfilt,
         py::arg("y").none(false), py::arg("hp_lambda").none(true) = NADBL,
         py::arg("trend").none(true) = false, py::arg("one_sided").none(true) = false)
    .def("hpfilt",
         [](GretlDataset &self, const std::string &y, double hp_lambda, bool one_sided) -> GretlMatrixPtr
         {
             return self.hpfilt(y, hp_lambda, false, one_sided);
         }, py::arg("y").none(false), py::arg("hp_lambda").none(true) = NADBL, py::arg("one_sided").none(true) = false)
    .def("bwfilt", &GretlDataset::bwfilt,
         py::arg("y").none(false), py::arg("n").none(false), py::arg("omega").none(false))
    .def("bkfilt", &GretlDataset::bkfilt,
         py::arg("y").none(false), py::arg("f1").none(true) = 0,
         py::arg("f2").none(true) = 0, py::arg("k").none(true) = 0)
    .def("get_accessor", &GretlDataset::get_accessor,
         py::arg("name").none(false))
    .def("corr", &GretlDataset::corr<std::string>,
         py::arg("varlist").none(true) = "", py::kw_only())
    .def("corr", &GretlDataset::corr<py::list>,
         py::arg("varlist").none(true) = py::list(), py::kw_only())
    .def("cov", &GretlDataset::cov<std::string>,
         py::arg("vars").none(true) = "")
    .def("cov", &GretlDataset::cov<py::list>,
         py::arg("vars").none(true) = py::list())
    .def("hurst", &GretlDataset::hurst,
         py::arg("series").none(false))
    .def("xcorrgm",
         [](GretlDataset &self, const std::string &varlist, int order, const py::kwargs &kwargs) -> py::object
         {
             return self.xcorrgm(varlist, order, kwargs);
         }, py::arg("varlist").none(false), py::arg("order").none(true) = 0)
    .def("xcorrgm",
         [](GretlDataset &self, const std::string &series1, const std::string &series2, int order, const py::kwargs &kwargs) -> py::object
         {
             return self.xcorrgm(series1 + " " + series2, order, kwargs);
         }, py::arg("series1").none(false), py::arg("series2").none(false), py::arg("order").none(true) = 0)
    .def("xcorrgm", &GretlDataset::xcorrgm<py::list>,
         py::arg("vars").none(false), py::arg("order").none(true) = 0, py::kw_only())
    .def("pca", &GretlDataset::pca<std::string>,
         py::arg("varlist").none(true) = "", py::kw_only())
    .def("pca", &GretlDataset::pca<py::list>,
         py::arg("varlist").none(true) = py::list(), py::kw_only())
    .def("kmeans", py::overload_cast<const std::string&, int, const GretlBundlePtr>(&GretlDataset::kmeans<std::string>),
         py::arg("X").none(false), py::arg("centers").none(false),
         py::arg("opts").none(true) = nullptr)
    .def("kmeans", py::overload_cast<const py::list&, int, const GretlBundlePtr>(&GretlDataset::kmeans<py::list>),
         py::arg("X").none(false), py::arg("centers").none(false),
         py::arg("opts").none(true) = nullptr)
    .def("kmeans", py::overload_cast<const std::string&, const GretlMatrix&, const GretlBundlePtr>(&GretlDataset::kmeans<std::string>),
         py::arg("X").none(false), py::arg("centers").none(false),
         py::arg("opts").none(true) = nullptr)
    .def("kmeans", py::overload_cast<const py::list&, const GretlMatrix&, const GretlBundlePtr>(&GretlDataset::kmeans<py::list>),
         py::arg("X").none(false), py::arg("centers").none(false),
         py::arg("opts").none(true) = nullptr)
    .def("xtab", &GretlDataset::xtab<std::string>,
         py::arg("varlist").none(true) = "", py::kw_only())
    .def("xtab", &GretlDataset::xtab<py::list>,
         py::arg("varlist").none(true) = py::list(), py::kw_only());

    cls.attr("__module__") = "gretl";
}
