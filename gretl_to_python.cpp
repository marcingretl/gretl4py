#include "gretl_calls.h"
#include <pybind11/numpy.h>

class Module
{
    public:
        Module() { module_init(); }
        ~Module() { module_cleanup(); }
};

const std::string info = "This is a help text for module.";

PYBIND11_MODULE(gretl, m) {
    static Module module;
    m.doc() = info;
    m.def("about", &about);

    // data management
    m.def("load_data", py::overload_cast<const py::array_t<double> &> (&load_data));
    m.def("load_data", py::overload_cast<const py::dict &> (&load_data));
    m.def("printdata", py::overload_cast<const py::dict &, bool> (&printdata),
          py::arg("data").none(false),
          py::arg("varamesonly") = false);
    m.def("printdata", py::overload_cast<bool> (&printdata),
          py::arg("varamesonly") = false);
    m.def("get_gretl_data", &get_gretl_data, py::arg("fname").none(false));

    // ARMA estimation
    m.def("arma",
	  py::overload_cast<const py::dict &, const std::string,
          int, int, int, bool> (&arma),
          py::arg("data").none(false),
          py::arg("y").none(false),
          py::arg("p") = 1,
          py::arg("d") = 0,
          py::arg("q") = 0,
          py::arg("print") = false,
          py::return_value_policy::take_ownership);
    m.def("arma",
	  py::overload_cast<const std::string, int, int, int, bool> (&arma),
          py::arg("y").none(false),
          py::arg("p") = 1,
          py::arg("d") = 0,
          py::arg("q") = 0,
          py::arg("print") = false,
          py::return_value_policy::take_ownership);

    // OLS estimation
    m.def("ols",
          py::overload_cast<const std::string, const py::dict &, bool> (&ols),
          py::arg("formula").none(false),
          py::arg("data").none(false),
          py::arg("print") = false,
          py::return_value_policy::take_ownership);
    m.def("ols",
          py::overload_cast<const std::string, bool> (&ols),
          py::arg("formula").none(false),
          py::arg("print") = false,
          py::return_value_policy::take_ownership);

    // Logit and probit estimation
    m.def("logit", &logit,
          py::arg("formula").none(false),
          py::arg("data").none(false),
          py::arg("print") = false,
          py::return_value_policy::take_ownership);
    m.def("probit", &probit,
          py::arg("formula").none(false),
          py::arg("data").none(false),
          py::arg("print") = false,
          py::return_value_policy::take_ownership);

    // libset setting and reporting
    m.def("set",
          py::overload_cast<const std::string, const std::string> (&set),
          py::arg("key").none(false),
          py::arg("val").none(false));
    m.def("set",
          py::overload_cast<const std::string, int> (&set),
          py::arg("key").none(false),
          py::arg("val").none(false));
    m.def("set",
          py::overload_cast<const std::string> (&set),
          py::arg("key").none(false));
}
