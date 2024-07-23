#include <gretl/libgretl.h>

namespace py = pybind11;

void gretl_model_to_dict (MODEL *model, DATASET *dset, py::dict &DICT);

void numpy_dict_to_gretl_dset (const py::dict &DICT, DATASET *dset);

void gretl_dset_to_numpy_dict (char *fname, py::dict &pydict);

// Note: only one is external
void numpy_array_to_gretl_storage (const py::array_t<double> &pynum_array, gretl_matrix *m);
