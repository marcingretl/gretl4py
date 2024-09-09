#include <pybind11/pybind11.h>
#include <iostream> // these two will be moved to gretl4py_data.cpp
#include <pybind11/stl.h>
#include <gretl/libgretl.h>

namespace py = pybind11;

int gretl_model_to_dict (MODEL *model, DATASET *dset, const char *summary, py::dict &DICT);

void numpy_dict_to_gretl_dset (const py::dict &DICT, DATASET *dset);

void gretl_dset_to_numpy_dict (const std::string fname, py::dict &pydict);

DATASET *gretl_load_global_dataset (const std::string fname);

class MapToPy
{
protected:
    const py::object *res;
    std::string as_str;
public:
    MapToPy (void)
    {
        res = nullptr;
        as_str.clear();
    }
    MapToPy (const std::optional<py::object> &pyobject)
    {
        res = &*pyobject;

        if (py::isinstance<py::str>(*pyobject)) {
            as_str = py::cast<std::string>(*pyobject);
        } else if (py::isinstance<py::int_>(*pyobject)) {
            as_str = std::to_string(py::cast<int>(*pyobject));
        } else {
            as_str.clear();
            // DEBUG !!!
            std::cerr << py::cast<std::string>(pyobject->get_type()) + " to string?";
            std::cerr << std::endl;
        }
    }

    auto *ptr (void) { return res; }

    std::string to_string (void)
    {
        //return (as_str.empty() ? nullptr : as_str); /* better? */
        return as_str;
    }
};

class MapToPyDict : public MapToPy
{
private:
    using MapToPy::res;
    py::dict dict;
public:
    MapToPyDict(const std::optional<std::map<std::string, py::object>> &data)
    {
        for (const auto &item : *data) {
            dict[py::cast(item.first)] = item.second;
        }
        res = &dict;
    }
};
