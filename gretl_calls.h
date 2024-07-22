#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#define GRETL_MODULE_VERSION 0.1

namespace py = pybind11;

void module_init (void);

void module_cleanup (void);

void about (void);

void load_data (const py::array_t<double> &pynum_array);
void load_data (const py::dict &pydict);

void printdata (const py::dict &data, bool varamesonly);
void printdata (bool varamesonly);

py::dict *get_gretl_data (const std::string fname);

py::dict *arma (const py::dict &data, const std::string y,
                      int p, int d, int q, bool print);
py::dict *arma (const std::string y, int p, int d, int q, bool print);

py::dict *ols (const std::string formula, const py::dict &data,bool print);
py::dict *ols (const std::string formula, bool print);

py::dict *logit (const std::string formula, const py::dict &data, bool print);

py::dict *probit (const std::string formula, const py::dict &data, bool print);

void set (const std::string key, const std::string val);
void set (const std::string key, int val);
void set (const std::string key);
