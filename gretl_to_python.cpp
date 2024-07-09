#include "gretl_calls.h"
#include <pybind11/numpy.h>

const char *about = "This is a help text for module.";

PYBIND11_MODULE(gretl, m) {
    m.doc() = about;
    m.def("dsettest", &dsettest);
    m.def("load_matrix", &load_matrix, "to be written");
    m.def("load_data", &load_data, "to be written");
    m.def("arma_estimate", &arma_estimate, "to be written",
          pybind11::arg("p") = 1,
          pybind11::arg("d") = 0,
          pybind11::arg("q") = 0,
          pybind11::arg("do_print") = false);
}
