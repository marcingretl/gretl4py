#include <pybind11/stl.h>
#include <gretl/libgretl.h>
#include <iostream>

namespace py = pybind11;

#define is_pybool(o) (py::isinstance<py::bool_>(o))
#define is_pyint(o) (py::isinstance<py::int_>(o))
#define is_pyfloat(o) (py::isinstance<py::float_>(o))
#define is_pystr(o) (py::isinstance<py::str>(o))
#define is_pydict(o) (py::isinstance<py::dict>(o))
#define is_pytuple(o) (py::isinstance<py::tuple>(o))

int gretl_model_to_dict (MODEL *model, DATASET *dset, const char *summary, py::dict &DICT);

void handle_gretl_error (int err);

PRN *get_buffered_printer (void);

void finalize_buffered_printer (PRN *prn);

//
// Class to handle gretl 'DATASET' type
//
class GretlDataset
{
private:
    inline void allocate_dset (void)
    {
        free_dset();
        dset = datainfo_new();
    }

    inline void free_dset (void)
    {
        if (dset) {
            destroy_dataset(dset);
            dset = nullptr;
        }
    }
public:
    DATASET *dset = nullptr;

    GretlDataset (const std::string fname); // we load dataset from file
    GretlDataset (const py::dict &pydict);  // we load dataset from Python's dictionary
    ~GretlDataset (void);

    py::dict *to_dict (void); // memory must be freed at the caller level

    void printdset (bool namesonly = false);

    void sample (const py::kwargs &kwargs);
};

//
// Class to handle 'gretl_bundle' type
//
class GretlBundle
{
private:
    gretl_bundle *b;
    char *json_buf = nullptr;
    bool free_b = true;
public:
    int myerr = 0;

    GretlBundle (void);                         // we get contents of $sysinfo bundle
    GretlBundle (MODEL *model, DATASET *dset);  // we produce bundle from MODEL struct
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
   MapToPy (const std::optional<py::object> &pyobject); // currently std::string and std::int are supported

   auto *ptr (void) { return res; }

   std::string to_string (void) { return as_str; }
};
