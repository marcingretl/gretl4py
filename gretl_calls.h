#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

void about (const char *info);

void load_data (const pybind11::array_t<double> &pynum_array);
void load_data (const pybind11::dict &pydict);

pybind11::dict arma (const pybind11::dict &data,
                     int p, int d, int q, bool print);
pybind11::dict arma (int p, int d, int q, bool print);
