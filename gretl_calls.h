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
void load_data (const py::dict &pydict);
void numpy_array_test (const py::array_t<double> &pynum_array);

void printdata (const py::dict &data, bool namesonly);
void printdata (bool namesonly);

py::dict *get_gretl_data (const std::string fname);
void attach_gretl_data (const std::string fname);

// model estimation
py::dict *arma (const std::string y, const py::tuple &tup, const py::kwargs &kwargs);

py::dict *ols (const std::string formula, const py::kwargs &kwargs);

py::dict *wls (const std::string formula, const py::kwargs &kwargs);

py::dict *logit (const std::string formula, const py::kwargs &kwargs);

py::dict *probit (const std::string formula, const py::kwargs &kwargs);

py::dict *poisson (const std::string formula, const py::kwargs &kwargs);

py::dict *negbin (const std::string formula, const py::kwargs &kwargs);

// content from automation/more_calls.h

py::dict *biprobit (const std::string formula, const py::kwargs &kwargs);

py::dict *duration (const std::string formula, const py::kwargs &kwargs);

py::dict *heckit (const std::string formula, const py::kwargs &kwargs);

py::dict *logistic (const std::string formula, const py::kwargs &kwargs);

py::dict *ar (const std::string formula, const py::kwargs &kwargs);

py::dict *ar1 (const std::string formula, const py::kwargs &kwargs);

py::dict *lad (const std::string formula, const py::kwargs &kwargs);

py::dict *garch (const std::string formula, const py::kwargs &kwargs);

py::dict *mpols (const std::string formula, const py::kwargs &kwargs);

py::dict *panel (const std::string formula, const py::kwargs &kwargs);

py::dict *tsls (const std::string formula, const py::kwargs &kwargs);
