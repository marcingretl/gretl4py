#include "gretl_calls.h"
#include <pybind11/numpy.h>

class Module
{
public:
    Module()
    {
        module_init();
    }
    ~Module()
    {
        module_cleanup();
    }
};

const char *info = "This is a help text for module.";

PYBIND11_MODULE(gretl, m) {
    static Module module;
    m.doc() = info;
    m.def("about", &about, pybind11::arg("info") = info);
    m.def("load_data", pybind11::overload_cast<const pybind11::array_t<double> &>(&load_data));
    m.def("load_data", pybind11::overload_cast<const pybind11::dict &>(&load_data));
    m.def("arma", pybind11::overload_cast<const pybind11::dict &,
          int, int, int, bool>(&arma), "some day",
          pybind11::arg("data").none(false),
          pybind11::arg("p") = 1,
          pybind11::arg("d") = 0,
          pybind11::arg("q") = 0,
          pybind11::arg("print") = false);
    m.def("arma", pybind11::overload_cast<int, int, int, bool>(&arma), "some day",
          pybind11::arg("p") = 1,
          pybind11::arg("d") = 0,
          pybind11::arg("q") = 0,
          pybind11::arg("print") = false);
}
