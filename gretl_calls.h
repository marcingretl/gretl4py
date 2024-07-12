#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

pybind11::dict arma_estimate (int p, int d, int q, bool do_print);

pybind11::dict dsettest (int p, int d, int q, bool do_print,
                         const pybind11::dict &pydict);

void load_data (const pybind11::array_t<double> &pynum_array);
void load_data (const pybind11::dict &pydict);
