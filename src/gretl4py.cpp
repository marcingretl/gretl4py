#include "gretl4py.h"
#include <pybind11/numpy.h>

class Module
{
    public:
        Module() { module_init(); }
        ~Module() { module_cleanup(); }
};

const std::string info = "This is a help text for module.";

PYBIND11_MODULE(_gretl, m) {
    // module related tools
    static Module module;
    m.doc() = info;
    m.def("about", &about,
          py::arg("do_print") = true);
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
    m.def("attach_data", &attach_data,
          py::arg("pydict").none(false));
    m.def("printdata",
          py::overload_cast<const py::dict &, bool> (&printdata),
          py::arg("data").none(false),
          py::arg("namesonly") = false);
    m.def("printdata",
          py::overload_cast<bool> (&printdata),
          py::arg("namesonly") = false);
    m.def("get_gretl_data", &get_gretl_data, py::arg("fname").none(false));
    m.def("attach_gretl_data", &attach_gretl_data, py::arg("fname").none(false));

    // sampling (for gretl datasets only)
    m.def("sample", &sample);

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
    // WLS
    m.def("wls", &wls,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    // Logit and probit
    m.def("logit", &logit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("probit", &probit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    // Count models
    m.def("poisson", &poisson,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("negbin", &negbin,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    // content from automation/more_mod.cpp
    m.def("biprobit", &biprobit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("duration", &duration,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("heckit", &heckit,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("logistic", &logistic,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("ar", &ar,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("ar1", &ar1,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("lad", &lad,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("garch", &garch,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("mpols", &mpols,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("panel", &panel,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
    m.def("tsls", &tsls,
          py::arg("formula").none(false),
          py::return_value_policy::take_ownership);
}
