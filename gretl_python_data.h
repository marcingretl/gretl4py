#include <gretl/libgretl.h>

void gretl_model_to_dict (const MODEL *model, pybind11::dict &DICT);

void numpy_dict_to_gretl_dset (const pybind11::dict &DICT, DATASET *dset);

// this function will be removed from header!!!
void numpy_array_to_gretl_storage (const pybind11::array_t<double> &pynum_array,
                                   gretl_matrix *m);
