#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#define GRETL_MODULE_VERSION 0.1

namespace py = pybind11;

// module related tools
void module_init (void);

void module_cleanup (void);

py::dict about (bool do_print = false);

/*
void set (const std::string key, const std::string val);
void set (const std::string key, int val);
void set (const std::string key);*/
void set (const std::string key,
          const std::optional<py::object> &pyval = std::nullopt);

// data management
void attach_data (const py::dict &pydict);

/*void printdata (const py::dict &data, bool namesonly);
void printdata (bool namesonly);*/
void printdata (const std::optional<py::dict> &data = std::nullopt,
                bool namesonly = false);

py::dict *get_gretl_data (const std::string fname);
void attach_gretl_data (const std::string fname);

void sample (const py::kwargs &kwargs);

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
