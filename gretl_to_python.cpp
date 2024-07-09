#include "gretl_calls.h"

const char *about = "This is a help text for module.";

PYBIND11_MODULE(gretl, m) {
    m.doc() = about;
    m.def("arma_estimate", &arma_estimate, "to be written",
          pybind11::arg("p") = 1,
          pybind11::arg("d") = 0,
          pybind11::arg("q") = 0,
          pybind11::arg("do_print") = false);
    m.def("numpy_array_to_gretl_data", &numpy_array_to_gretl_data);
}
