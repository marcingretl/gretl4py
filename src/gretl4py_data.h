#include <pybind11/stl.h>
#include <gretl/libgretl.h>

namespace py = pybind11;

int gretl_model_to_dict (MODEL *model, DATASET *dset, const char *summary, py::dict &DICT);

void numpy_dict_to_gretl_dset (const py::dict &DICT, DATASET *dset);

void gretl_dset_to_numpy_dict (const std::string fname, py::dict &pydict);

DATASET *gretl_load_global_dataset (const std::string fname);

// std::optional to pybind11 object mapper
class MapToPy
{
private:
    const py::object *res;
    std::string as_str;
    //py::dict dict;
public:
    MapToPy (const std::optional<py::object> &pyobject);
    /*
    MapToPy (const std::optional<std::map<std::string, py::object>> &data)
    {
        for (const auto &item : *data) {
            dict[py::cast(item.first)] = item.second;
        }
        res = &dict;
        as_str.clear();
    }
    */

    auto *ptr (void) { return res; }

    std::string to_string (void) { return as_str; }
};
