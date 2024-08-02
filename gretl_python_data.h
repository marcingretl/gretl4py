#include <gretl/libgretl.h>

namespace py = pybind11;

void gretl_model_to_dict (MODEL *model, DATASET *dset, char *buf, py::dict &DICT);

void numpy_dict_to_gretl_dset (const py::dict &DICT, DATASET *dset);

void gretl_dset_to_numpy_dict (const std::string fname, py::dict &pydict);

DATASET *gretl_load_global_dataset (const std::string fname);
