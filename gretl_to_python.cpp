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
    // module related tools
    static Module module;
    m.doc() = info;
    m.def("about", &about);
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

    // data management
    m.def("load_data", py::overload_cast<const py::array_t<double> &> (&load_data));
    m.def("load_data", py::overload_cast<const py::dict &> (&load_data));
    m.def("printdata", py::overload_cast<const py::dict &, bool> (&printdata),
          py::arg("data").none(false),
          py::arg("varamesonly") = false);
    m.def("printdata", py::overload_cast<bool> (&printdata),
          py::arg("varamesonly") = false);
    m.def("get_gretl_data", &get_gretl_data, py::arg("fname").none(false));
    m.def("attach_gretl_data", &attach_gretl_data, py::arg("fname").none(false));

    // model estimation
    // ARMA
    m.def("arma", &arma,
          py::arg("y").none(false),
          py::arg("order").none(false),
          py::return_value_policy::take_ownership);
    // OLS
    m.def("ols", &ols,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    // Logit and probit
    m.def("logit", &logit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("probit", &probit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
}
