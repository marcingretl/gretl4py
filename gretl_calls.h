#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#define GRETL_MODULE_VERSION 0.1

namespace py = pybind11;

// module related tools
void module_init (void);

void module_cleanup (void);

void about (void);

void set (const std::string key, const std::string val);
void set (const std::string key, int val);
void set (const std::string key);

// data management
void load_data (const py::array_t<double> &pynum_array);
void load_data (const py::dict &pydict);

void printdata (const py::dict &data, bool namesonly);
void printdata (bool namesonly);

py::dict *get_gretl_data (const std::string fname);

// model estimation
py::dict *arma (const std::string y, const py::tuple &tup, const py::kwargs &kwargs);

py::dict *ols (const std::string formula, const py::kwargs &kwargs);

py::dict *logit (const std::string formula, const py::kwargs &kwargs);

py::dict *probit (const std::string formula, const py::kwargs &kwargs);

