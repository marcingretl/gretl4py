#include <pybind11/stl.h>
#include <gretl/libgretl.h>

namespace py = pybind11;

#define is_pybool(o) (py::isinstance<py::bool_>(o))
#define is_pyint(o) (py::isinstance<py::int_>(o))
#define is_pyfloat(o) (py::isinstance<py::float_>(o))
#define is_pystr(o) (py::isinstance<py::str>(o))
#define is_pydict(o) (py::isinstance<py::dict>(o))
#define is_pytuple(o) (py::isinstance<py::tuple>(o))

int gretl_model_to_dict (MODEL *model, DATASET *dset, const char *summary, py::dict &DICT);

void numpy_dict_to_gretl_dset (const py::dict &DICT, DATASET *dset);

void gretl_dset_to_numpy_dict (const std::string fname, py::dict &pydict);

DATASET *gretl_load_global_dataset (const std::string fname);

//
// This is general purpose py::object to py::<type> mapper
//
class MapToPy
{
private:
    const py::object *res;
    std::string as_str;
public:
   MapToPy (const std::optional<py::object> &pyobject);

   auto *ptr (void) { return res; }

   std::string to_string (void) { return as_str; }
};

//
// Class to handle 'gretl_bundle' type
//
class GretlBundle
{
private:
    gretl_bundle *b;
    PRN *prn = nullptr;
    char *json_buf = nullptr;
    bool free_b = true;
public:
    int err = 0;

    GretlBundle (void);
    GretlBundle (MODEL *model, DATASET *dset);

    ~GretlBundle (void);

    char *to_json (void);

    void print (void);
};
