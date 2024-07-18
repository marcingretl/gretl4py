#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#define GRETL_MODULE_VERSION 0.1

void module_init (void);
void module_cleanup (void);
void about (const std::string &info);

void load_data (const pybind11::array_t<double> &pynum_array);
void load_data (const pybind11::dict &pydict);

pybind11::dict *arma (const pybind11::dict &data, const std::string y,
                      int p, int d, int q, bool print);
pybind11::dict *arma (const std::string y, int p, int d, int q, bool print);
