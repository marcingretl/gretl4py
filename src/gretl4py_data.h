/*
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <pybind11/stl.h>
#include <gretl/libgretl.h>
#include <iostream>

namespace py = pybind11;

typedef std::vector<std::variant<double, std::string, int>> vgretl_vector;
typedef std::vector<vgretl_vector> vgretl_matrix;

#define is_pybool(o) (py::isinstance<py::bool_>(o))
#define is_pyint(o) (py::isinstance<py::int_>(o))
#define is_pyfloat(o) (py::isinstance<py::float_>(o))
#define is_pystr(o) (py::isinstance<py::str>(o))
#define is_pydict(o) (py::isinstance<py::dict>(o))
#define is_pytuple(o) (py::isinstance<py::tuple>(o))

int gretl_model_to_dict (MODEL *model, DATASET *dset,
                         const char *summary, py::dict &DICT);

void handle_gretl_error (int err);

//
// Class to handle gretl 'DATASET' type
//
class GretlDataset
{
private:
    inline void allocate_dset (void)
    {
        GretlDataset::free_dset();
        dset = datainfo_new();
    }

    inline void free_dset (void)
    {
        if (dset) {
            destroy_dataset(dset);
            dset = NULL;
        }
    }
public:
    int err = 0;
    DATASET *dset = NULL;

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
    bool free_b = true;
    char *json_buf = NULL;
    gretl_bundle *b;
    py::dict *as_dict = nullptr;
public:
    int err = 0;

    GretlBundle (void);                         // we get contents of $sysinfo bundle
    GretlBundle (MODEL *model, DATASET *dset);  // we produce bundle from MODEL struct
    ~GretlBundle (void);

    py::dict *to_dict (void);                 // bundle as dict: memory must be freed at the caller level

    void print (void);
};

//
// This is general purpose py::object/py::handle to C++ types mapper
//
class MapPyToCpp
{
private:
    std::string as_str;

    template<typename T>
    void convert (const T *pyobj);
public:
    int err = 0;

    MapPyToCpp (const std::optional<py::object> &pyobject);
    MapPyToCpp (const py::handle &pyhandle);

    //auto *ptr (void) { return res; } // not used

    std::string to_string (void) { return as_str; }

    std::string_view to_string_view (void) { return static_cast<std::string_view>(as_str); }
};

//
// Class to handle printing
//
class GretlPrint
{
public:
    int err = 0;
    PRN *prn = NULL;

    GretlPrint (PrnType prn_type = GRETL_PRINT_BUFFER);  // by default we use buffered printer
    ~GretlPrint (void);

    void print (void);
};
