#include <gretl/libgretl.h>

void gretl_model_to_dict (const MODEL *model, pybind11::dict &DICT);

void numpy_array_to_gretl_matrix (const pybind11::array_t<double> &pynum_array,
                                  gretl_matrix *m);

void numpy_dict_to_gretl_dset (const pybind11::dict &DICT, DATASET *dset);

//void numpy_array_to_gretl_data (const pybind11::array_t<double> &pynum_array); /* for testing only: to be deleted!!! */


