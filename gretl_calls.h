#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

void numpy_array_to_gretl_data (const pybind11::array_t<double> &pynum_array);

pybind11::dict arma_estimate (int p, int d, int q, bool do_print);