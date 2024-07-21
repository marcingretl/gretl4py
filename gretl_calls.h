#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#define GRETL_MODULE_VERSION 0.1

void module_init (void);

void module_cleanup (void);

void about (void);

void load_data (const pybind11::array_t<double> &pynum_array);
void load_data (const pybind11::dict &pydict);

void printdata (const pybind11::dict &data, bool varamesonly);
void printdata (bool varamesonly);

pybind11::dict *get_gretl_data (const std::string fname);

pybind11::dict *arma (const pybind11::dict &data,
		      const std::string y,
		      int p, int d, int q,
		      bool print);

pybind11::dict *arma (const std::string y,
		      int p, int d, int q,
		      bool print);

pybind11::dict *ols (const std::string formula,
		     const pybind11::dict &data,
		     bool print);

pybind11::dict *ols (const std::string formula,
		     bool print);

pybind11::dict *logit (const std::string formula,
		       const pybind11::dict &data,
		       bool print);

pybind11::dict *probit (const std::string formula,
			const pybind11::dict &data,
			bool print);
