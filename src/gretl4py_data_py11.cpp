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

template <typename T>
py::array_t<T> make_numpy_view_impl (const GretlMatrix &m, py::handle owner, T *ptr)
{
    //
    // helper for making NumPy view of GretlMatrix
    //
    if (m.is_vector()) {
        ssize_t n = std::max(m.rows(), m.cols());

        return py::array_t<T>({n}, {sizeof(T)}, ptr, owner);
    }

    return py::array_t<T>({m.rows(), m.cols()}, {sizeof(T), sizeof(T) * m.rows()}, ptr, owner);
}

template <bool ROWS>
py::object matrix_row_cols_names_get(const GretlMatrix &m, std::optional<int> index)
{
    //
    // helper for retrieving matrix row/column names.
    //
    auto names = m.rows_cols_names_get<ROWS>();

    if (!index) return names;
    int i = *index;

    if (i < 0) throw py::index_error("list index out of range");

    return py::str(names[i]);
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
            [](const GretlDataset &self)
            {
                return GretlDataset::clone(self);
            },
            R"doc(Return a deep copy of the dataset.)doc")
    .def("__deepcopy__", [](const GretlDataset &self, py::dict) { return GretlDataset::clone(self); })
    .def("__repr__", &GretlDataset::repr)
    .def("__getitem__", py::overload_cast<const py::slice&>(&GretlDataset::getitem_impl))
    .def("__getitem__", py::overload_cast<const std::string&>(&GretlDataset::getitem_impl))
    .def("__getitem__", py::overload_cast<const py::list&>(&GretlDataset::getitem_impl))
    .def("clone", [](const GretlDataset &self) { return GretlDataset::clone(self); })
    .def("head", &head_tail_print<HEAD>, py::arg("nobs") = 5, py::kw_only())
    .def("tail", &head_tail_print<TAIL>, py::arg("nobs") = 5, py::kw_only())
    .def("set_as_default", &GretlDataset::set_as_default)
    .def("summary",
            [](const GretlDataset &self)
            {
                self.summary();
            })
    .def("sample",
            [](GretlDataset &self, const std::string &startobs,
            const std::string &endobs, const py::kwargs &kwargs)
            {
                // direct call to GretlDataset::smpl()
                self.smpl(startobs, endobs, kwargs);
            }, py::arg("startobs").none(true) = "", py::arg("endobs").none(true) = "", py::kw_only())
    .def("sample",
            [](GretlDataset &self, const std::string &startobs, const py::kwargs &kwargs)
            {
                self.smpl(startobs, "", kwargs);
            }, py::arg("startobs").none(false), py::kw_only())
    .def("sample",
            [](GretlDataset &self, int startobs, int endobs, const py::kwargs &kwargs)
            {
                std::string startobs_str = startobs == NADBL ? "" : std::to_string(startobs);
                std::string endobs_str = endobs == NADBL ? "" : std::to_string(endobs);

                self.smpl(startobs_str, endobs_str, kwargs);
            }, py::arg("startobs").none(true) = NADBL, py::arg("endobs").none(true) = NADBL, py::kw_only())
    .def("sample",
            [](GretlDataset &self, int startobs, const py::kwargs &kwargs)
            {
                self.smpl(std::to_string(startobs), "", kwargs);
            }, py::arg("startobs").none(false), py::kw_only())
    .def("setobs",
            [](GretlDataset &self, std::string &periodicity, const std::string &startobs, const py::kwargs &kwargs)
            {
                self.setobs(periodicity, startobs, kwargs);
            }, py::arg("periodicity").none(false), py::arg("startobs").none(false), py::kw_only())
    .def("setobs",
            [](GretlDataset &self, int periodicity, const std::string &startobs, const py::kwargs &kwargs)
            {
                self.setobs(std::to_string(periodicity), startobs, kwargs);
            }, py::arg("periodicity").none(false), py::arg("startobs").none(false), py::kw_only())
    .def("setobs",
            [](GretlDataset &self, int periodicity, int startobs, const py::kwargs &kwargs)
            {
                self.setobs(std::to_string(periodicity), std::to_string(startobs), kwargs);
            }, py::arg("periodicity").none(false), py::arg("startobs").none(false), py::kw_only())
    .def("addobs",
            [](GretlDataset &self, int n, const py::kwargs &kwargs)
            {
                self.do_modify_dataset<std::nullptr_t>(DS_ADDOBS, nullptr, std::to_string(n), kwargs);
            }, py::arg("n").none(false), py::kw_only())
    .def("insobs",
            [](GretlDataset &self, int n)
            {
                self.do_modify_dataset<std::nullptr_t>(DS_INSOBS, nullptr, std::to_string(n), py::dict());
            }, py::arg("n").none(false))
    .def("compact",
            [](GretlDataset &self, py::object freq, const std::string &method)
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

                self.do_modify_dataset<std::nullptr_t>(DS_COMPACT, nullptr, formula, py::dict());
            }, py::arg("start").none(false), py::arg("method").none(true) = "")
    .def("expand",
            [](GretlDataset &self, int freq)
            {
                self.do_modify_dataset<std::nullptr_t>(DS_EXPAND, nullptr, std::to_string(freq), py::dict());
            }, py::arg("n").none(true) = -999)
    .def("transpose",
            [](GretlDataset &self)
            {
                self.do_modify_dataset<std::nullptr_t>(DS_TRANSPOSE, nullptr, "", py::dict());
            })
    .def("sortby",
            [](GretlDataset &self, const std::string &varlist)
            {
                self.do_modify_dataset<std::string>(DS_SORTBY, varlist, "", py::dict());
            }, py::arg("varlist").none(false))
    .def("sortby",
            [](GretlDataset &self, const py::list &varlist)
            {
                self.do_modify_dataset<py::list>(DS_SORTBY, varlist, "", py::dict());
            }, py::arg("varlist").none(false))
    .def("dsortby",
            [](GretlDataset &self, const std::string &varlist)
            {
                self.do_modify_dataset<std::string>(DS_DSORTBY, varlist, "", py::dict());
            }, py::arg("varlist").none(false))
    .def("dsortby",
            [](GretlDataset &self, const py::list &varlist)
            {
                self.do_modify_dataset<py::list>(DS_DSORTBY, varlist, "", py::dict());
            }, py::arg("varlist").none(false))
    .def("resample",
            [](GretlDataset &self, int n)
            {
                self.do_modify_dataset<std::nullptr_t>(DS_RESAMPLE, nullptr, std::to_string(n), py::dict());
            }, py::arg("n").none(false))
    .def("pad_daily",
            [](GretlDataset &self, int days)
            {
                self.do_modify_dataset<std::nullptr_t>(DS_PAD_DAILY, nullptr, std::to_string(days), py::dict());
            }, py::arg("days").none(false))
    .def("unpad_daily",
            [](GretlDataset &self)
            {
                self.do_modify_dataset<std::nullptr_t>(DS_UNPAD_DAILY, nullptr, "", py::dict());
            })
    .def("tdisagg", &GretlDataset::tdisagg,
            py::arg("Y").none(false), py::arg("s").none(false), py::kw_only())
    .def("to_dict", &GretlDataset::to_dict)
    .def("to_file", &GretlDataset::to_file,
            py::arg("filename").none(false),
            py::arg("varlist").none(true) = "", py::kw_only())
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
            [](GretlDataset &self, const std::string &series,
            const std::string &newname, const py::kwargs &kwargs)
            {
                self.rename(self.get_series_id(series, false), newname, kwargs);
            }, py::arg("series").none(false), py::arg("newname").none(false), py::kw_only())
    .def("rename",
            [](GretlDataset &self, const py::kwargs &kwargs)
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
    .def("print", &GretlDataset::print,
            py::arg("varlist").none(true) = "", py::kw_only())
    .def("get_series", &GretlDataset::get_series,
            py::arg("series").none(false))
    .def("hpfilt", &GretlDataset::hpfilt,
            py::arg("y").none(false), py::arg("hp_lambda").none(true) = NADBL,
            py::arg("trend").none(true) = false, py::arg("one_sided").none(true) = false)
    .def("hpfilt",
            [](GretlDataset &self, const std::string &y, double hp_lambda, bool one_sided)
            {
                return self.hpfilt(y, hp_lambda, false, one_sided);
            }, py::arg("y").none(false), py::arg("hp_lambda").none(true) = NADBL, py::arg("one_sided").none(true) = false)
    .def("bwfilt", &GretlDataset::bwfilt,
            py::arg("y").none(false), py::arg("n").none(false),
            py::arg("omega").none(false))
    .def("bkfilt", &GretlDataset::bkfilt,
            py::arg("y").none(false), py::arg("f1").none(true) = 0,
            py::arg("f2").none(true) = 0, py::arg("k").none(true) = 0)
    .def("get_accessor", &GretlDataset::get_accessor,
            py::arg("name").none(false))
    .def("corr", &GretlDataset::corr<std::string>,
            py::arg("varlist").none(true) = "", py::kw_only())
    .def("corr", &GretlDataset::corr<py::list>,
            py::arg("varlist").none(true) = py::list(), py::kw_only())
    .def("cov", &GretlDataset::cov<std::string>, py::arg("vars").none(true) = "")
    .def("cov", &GretlDataset::cov<py::list>, py::arg("vars").none(true) = py::list())
    .def("hurst", &GretlDataset::hurst, py::arg("series").none(false))
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
            py::arg("X").none(false), py::arg("centers").none(false), py::arg("opts").none(true) = nullptr)
    .def("kmeans", py::overload_cast<const py::list&, int, const GretlBundlePtr>(&GretlDataset::kmeans<py::list>),
            py::arg("X").none(false), py::arg("centers").none(false), py::arg("opts").none(true) = nullptr)
    .def("kmeans", py::overload_cast<const std::string&, const GretlMatrix&, const GretlBundlePtr>(&GretlDataset::kmeans<std::string>),
            py::arg("X").none(false), py::arg("centers").none(false), py::arg("opts").none(true) = nullptr)
    .def("kmeans", py::overload_cast<const py::list&, const GretlMatrix&, const GretlBundlePtr>(&GretlDataset::kmeans<py::list>),
            py::arg("X").none(false), py::arg("centers").none(false), py::arg("opts").none(true) = nullptr)
    .def("xtab", &GretlDataset::xtab<std::string>,
            py::arg("varlist").none(true) = "", py::kw_only())
    .def("xtab", &GretlDataset::xtab<py::list>,
            py::arg("varlist").none(true) = py::list(), py::kw_only())
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
            }, py::arg("type"), py::kw_only());

    cls.attr("__module__") = "gretl";
}

//
// GretlPackage mappings
//
void init_gretl4py_package_py11 (py::module_ &m)
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

//
// GretlBundle mappings
//
void init_gretl4py_bundle_py11 (py::module_ &m)
{
    auto cls = py::class_<GretlBundle, GretlBundlePtr>(m, "Bundle", py::dynamic_attr());

    cls
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
    .def("__setitem__", &GretlBundle::set_element<GretlMatrixPtr>)
    .def("__delitem__", &GretlBundle::delete_element)
    .def("pop", &GretlBundle::pop, py::arg("key").none(false))
    .def("keys", &GretlBundle::keys)
    .def("print", &GretlBundle::print<void>, py::arg("tree").none(true) = false)
    .def("to_dict", &GretlBundle::to_dict);

    cls.attr("__module__") = "gretl";
}

//
// GretlMatrix mappings
//
void init_gretl4py_gretl_matrix_py11 (py::module_ &m)
{
    auto cls = py::class_<GretlMatrix, GretlMatrixPtr>(m, "Matrix", py::dynamic_attr());

    cls
    .def(py::init<>())                                                              // default constructor
    .def(py::init<int, int, double, const std::string&>(),
         py::arg("rows").none(false), py::arg("cols").none(false),
         py::arg("fill").none(false) = 0.0, py::arg("type").none(false) = "real")   // filled real/complex matrix
    .def(py::init<int, int, const std::string&>(),
         py::arg("rows").none(false), py::arg("cols").none(false),
         py::arg("dist").none(false) = "normal")                                    // random matrix
    .def(py::init<double, double, double>(),
         py::arg("a").none(false), py::arg("b").none(false),
         py::arg("k").none(false) = 1.0)                                            // sequential row vector
    .def(py::init<int>(), py::arg("n").none(false))                                 // identity matrix
    .def(py::init<const py::array&>(), py::arg("array").none(false))                // from NumPy array
    .def(py::init<const py::sequence&>(), py::arg("seq").none(false))               // from Python sequence
    .def_static("eye", &GretlMatrix::identity,
                py::arg("n").none(false), py::arg("m") = std::nullopt)
    .def_static("diag", &GretlMatrix::diag, py::arg("seq").none(false))
    .def_static("zeros", &GretlMatrix::zeros,
                py::arg("rows").none(false), py::arg("cols").none(false))
    .def_property_readonly("rows",
                           [](const GretlMatrix &self)
                           {
                               return self.rows();
                           })
    .def_property_readonly("cols",
                           [](const GretlMatrix &self)
                           {
                               return self.cols();
                           })
    .def_property_readonly("is_complex",
                           [](const GretlMatrix &self)
                           {
                               return self.is_complex();
                           })
    .def_property_readonly("is_vector",
                           [](const GretlMatrix &self)
                           {
                               return self.is_vector();
                           })
    .def_property_readonly("numpy_view",
                           [](GretlMatrix &self) -> py::array
                           {
                               if (self.is_complex()) {
                                   return make_numpy_view_impl<std::complex<double>>(self,
                                                                                     py::cast(&self, py::return_value_policy::reference),
                                                                                     reinterpret_cast<std::complex<double>*>(self.z()));
                               }

                               return make_numpy_view_impl<double>(self, py::cast(&self, py::return_value_policy::reference), self.val());
                           })
    .def("__len__", &GretlMatrix::rows)
    .def("__array__",
         [](py::object self_obj, py::object , py::object ) -> py::array
         {
             //
             // to follow the protocol convention in 100% (dtype, copy)
             //
             auto &self = self_obj.cast<GretlMatrix&>();

             if (self.is_complex()) {
                 return make_numpy_view_impl<std::complex<double>>(self, py::cast(&self, py::return_value_policy::reference),
                                                                   reinterpret_cast<std::complex<double>*>(self.z()));
             }

             return make_numpy_view_impl<double>(self, py::cast(&self, py::return_value_policy::reference), self.val());
         }, py::arg("dtype") = py::none(), py::arg("copy") = py::none())
    .def("__copy__", [](const GretlMatrix &self) { return GretlMatrix(self); })
    .def("__deepcopy__", [](const GretlMatrix &self, py::dict) { return GretlMatrix(self); })
    .def("__repr__", &GretlMatrix::repr_impl)
    .def("__getitem__",
         [](const GretlMatrix &self, const py::object &idx) -> py::object
         {
             //
             // we dispatch slicing type
             //
             auto res = std::make_unique<GretlTypes>();

             if (is_pytuple(idx)) {
                 auto tuple = idx.cast<py::tuple>();

                 if (tuple.size() == 2 && (is_pyslice(tuple[0]) || is_pyslice(tuple[1]))) return variant_to_pyobject(self.get_slice(tuple));
                 if (tuple.size() == 2) return self.get_element(tuple[0].cast<int>(), tuple[1].cast<int>());
                 throw std::invalid_argument("Invalid matrix index");
             }
             if (is_pyint(idx)) return self.get_element(idx.cast<int>());
             if (is_pyslice(idx)) return variant_to_pyobject(self.get_slice(idx.cast<py::slice>()));
             //if (is_pystr(idx)) return variant_to_pyobject(self.get_slice(idx.cast<std::string>())); # slicing by string is not allowed for the time
             throw std::invalid_argument("Unsupported index type");
         })
    .def("__setitem__",
         [](GretlMatrix &self, std::pair<int, int> ij, double value) -> void
         {
             self.set_element<double>(ij.first, ij.second, value);
         }, py::arg("ij").none(false), py::arg("value").none(false))
    .def("__setitem__", py::overload_cast<int, double>(&GretlMatrix::set_element<double>),
         py::arg("i").none(false), py::arg("value").none(false))
    .def("__setitem__",
         [](GretlMatrix &self, std::pair<int, int> ij, std::complex<double> value) -> void
         {
             self.set_element<std::complex<double>>(ij.first, ij.second, value);
         }, py::arg("ij").none(false), py::arg("value").none(false))
    .def("__setitem__", py::overload_cast<int, std::complex<double>>(&GretlMatrix::set_element<std::complex<double>>),
         py::arg("i").none(false), py::arg("value").none(false))
    .def("__pos__", [](const GretlMatrix &self) { return GretlMatrix(self); })
    .def("__neg__", &GretlMatrix::neg)
    .def("__eq__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::EQ>)
    .def("__eq__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::EQ>)
    .def("__eq__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::EQ>)
    .def("__eq__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::EQ>)
    .def("__ne__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::NE>)
    .def("__ne__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::NE>)
    .def("__ne__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::NE>)
    .def("__ne__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::NE>)
    .def("__lt__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::LT>)
    .def("__lt__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::LT>)
    .def("__lt__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::LT>)
    .def("__lt__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::LT>)
    .def("__le__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::LE>)
    .def("__le__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::LE>)
    .def("__le__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::LE>)
    .def("__le__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::LE>)
    .def("__gt__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::GT>)
    .def("__gt__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::GT>)
    .def("__gt__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::GT>)
    .def("__gt__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::GT>)
    .def("__ge__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::GE>)
    .def("__ge__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::GE>)
    .def("__ge__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::GE>)
    .def("__ge__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::GE>)
    .def("__add__", &GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::ADD>)
    .def("__add__", &GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::ADD>)
    .def("__add__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::ADD>)
    .def("__add__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::ADD>)
    .def("__sub__", &GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::SUB>)
    .def("__sub__", &GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::SUB>)
    .def("__sub__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::SUB>)
    .def("__sub__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::SUB>)
    .def("__mul__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::MUL>)
    .def("__mul__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::MUL>)
    .def("__mul__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::MUL>)
    .def("__mul__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::MUL>)
    .def("__matmul__", &GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::MATMUL>)
    .def("__matmul__", &GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::MATMUL>)
    .def("__truediv__", &GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::TRUEDIV>)
    .def("__truediv__", &GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::TRUEDIV>)
    .def("__truediv__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::TRUEDIV>)
    .def("__truediv__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::TRUEDIV>)
    .def("__pow__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::POW>)
    .def("__pow__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::POW>)
    .def("__pow__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::POW>)
    .def("__pow__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::POW>)
    .def("__iadd__",
         [](GretlMatrix &self, const GretlMatrix &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::IADD>(rhs);
             } else {
                 return self.alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::IADD>(rhs);
             }
         }, py::arg("mat").none(false))
    .def("__iadd__",
         [](GretlMatrix &self, const py::array &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::IADD>(rhs);
             } else {
                 return self.alg_inplace_operations<py::array, GretlMatrixAlgOperations::IADD>(rhs);
             }
         }, py::arg("mat").none(false))
    .def("__iadd__", &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IADD>)
    .def("__iadd__", &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IADD>)
    .def("__isub__",
         [](GretlMatrix &self, const GretlMatrix &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::ISUB>(rhs);
             } else {
                 return self.alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::ISUB>(rhs);
             }
         }, py::arg("mat").none(false))
    .def("__isub__",
         [](GretlMatrix &self, const py::array &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::ISUB>(rhs);
             } else {
                 return self.alg_inplace_operations<py::array, GretlMatrixAlgOperations::ISUB>(rhs);
             }
         }, py::arg("mat").none(false))
    .def("__isub__", &GretlMatrix::alg_inplace_operations<py::array, GretlMatrixAlgOperations::ISUB>)
    .def("__isub__", &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::ISUB>)
    .def("__isub__", &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::ISUB>)
    .def("__imul__", &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IMUL>)
    .def("__imul__", &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IMUL>)
    .def("__itruediv__", &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::ITRUEDIV>)
    .def("__itruediv__", &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::ITRUEDIV>)
    .def("__ipow__",
         [](GretlMatrix &self, const GretlMatrix &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::IPOW>(rhs);
             } else {
                 return self.alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::IPOW>(rhs);

             }
         }, py::arg("mat").none(false))
    .def("__ipow__",
         [](GretlMatrix &self, const py::array &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::IPOW>(rhs);
             } else {
                 return self.alg_inplace_operations<py::array, GretlMatrixAlgOperations::IPOW>(rhs);
             }
         }, py::arg("mat").none(false))
    .def("__ipow__", &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IPOW>)
    .def("__ipow__", &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IPOW>)
    .def("__radd__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::ADD>)
    .def("__radd__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::ADD>)
    .def("__rsub__",
         [](const GretlMatrix &self, double val) -> GretlMatrixPtr
         {
             auto res = self.neg();

             res->alg_inplace_operations<double, GretlMatrixAlgOperations::IADD>(val);

             return res;
         })
    .def("__rsub__",
         [](const GretlMatrix &self, std::complex<double> val) -> GretlMatrixPtr
         {
             return self.neg()->alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::ADD>(val);
         })
    .def("__rmul__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::MUL>)
    .def("__rmul__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::MUL>)
    .def("__rtruediv__",
         [](const GretlMatrix &self, double val) -> GretlMatrixPtr
         {
             auto res = self.alg_dot_operations<double, GretlMatrixAlgOperations::TRUEDIV>(1.0);

             res->alg_inplace_operations<double, GretlMatrixAlgOperations::IMUL>(val);

             return res;
         })
    .def("__rtruediv__",
         [](const GretlMatrix &self, std::complex<double> val) -> GretlMatrixPtr
         {
             auto res = self.alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::TRUEDIV>(1.0);

             return res->alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::MUL>(val);
         })
    .def("copy", [](const GretlMatrix &self) { return GretlMatrix(self); })
    .def("to_numpy", &GretlMatrix::to_numpy)
    .def("rnameget", &matrix_row_cols_names_get<true>, py::arg("row").none(true) = std::nullopt)
    .def("cnameget", &matrix_row_cols_names_get<false>, py::arg("col").none(true) = std::nullopt)
    .def("rnameset", &GretlMatrix::rows_cols_names_set<true>, py::arg("names").none(false))
    .def("cnameset", &GretlMatrix::rows_cols_names_set<false>, py::arg("names").none(false));

    cls.attr("__module__") = "gretl";
}
