#include <gretl/libgretl.h>

void gretl_model_to_dict (MODEL *model, pybind11::dict &DICT);

void numpy_dict_to_gretl_dset (const pybind11::dict &DICT, DATASET *dset);

// Note: only one is external
void numpy_array_to_gretl_storage (const pybind11::array_t<double> &pynum_array,
                                   gretl_matrix *m);

//gretl_matrix *gretl_vcv_matrix_from_model (MODEL *pmod, const char *select, int *err);
