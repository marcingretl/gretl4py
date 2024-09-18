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
// Class to handle gretl 'DATASET' type
//
class GretlDataset
{
private:
    py::dict *as_dict = nullptr;    // dict representation
    bool free_as_dict = false;      // do we free 'as_dict'
    py::dict &tmp_dict = *as_dict;

    inline void allocate_dset (void)
    {
        if (dset != nullptr) {
            destroy_dataset(dset);
        }
        dset = datainfo_new();
    }

    void free_dict (void);

public:
    DATASET *dset = nullptr;
    int err = 0;

    GretlDataset (const std::string fname);
    GretlDataset (const py::dict &pydict);
    ~GretlDataset (void);

    py::dict *to_dict (bool do_free_dict = false);
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
