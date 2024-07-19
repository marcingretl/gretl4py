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
    m.def("load_data", pybind11::overload_cast<const pybind11::array_t<double> &> (&load_data));
    m.def("load_data", pybind11::overload_cast<const pybind11::dict &> (&load_data));
    m.def("get_gretl_data", &get_gretl_data, pybind11::arg("fname").none(false));
    m.def("arma",
	  pybind11::overload_cast<const pybind11::dict &, const std::string,
          int, int, int, bool> (&arma),
          pybind11::arg("data").none(false),
          pybind11::arg("y").none(false),
          pybind11::arg("p") = 1,
          pybind11::arg("d") = 0,
          pybind11::arg("q") = 0,
          pybind11::arg("print") = false,
          pybind11::return_value_policy::take_ownership);
    m.def("arma",
	  pybind11::overload_cast<const std::string, int, int, int, bool> (&arma),
          pybind11::arg("y").none(false),
          pybind11::arg("p") = 1,
          pybind11::arg("d") = 0,
          pybind11::arg("q") = 0,
          pybind11::arg("print") = false,
          pybind11::return_value_policy::take_ownership);
    m.def("ols",
	  pybind11::overload_cast<const std::string, const pybind11::dict &, bool> (&ols), 
          pybind11::arg("formula").none(false),
	  pybind11::arg("data").none(false),
	  pybind11::arg("print") = false,
          pybind11::return_value_policy::take_ownership);
    m.def("ols",
	  pybind11::overload_cast<const std::string, bool> (&ols),
          pybind11::arg("formula").none(false),
	  pybind11::arg("print") = false,
          pybind11::return_value_policy::take_ownership);    
}
