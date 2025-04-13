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
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GRETL4PY_DATA_H
#define GRETL4PY_DATA_H

#define SIMDJSON_DEVELOPMENT_CHECKS 0

#if defined(MB_VARIANT) && defined(__x86_64__) && defined(__APPLE__)
# define USE_MAPBOX_VARIANT 1
#else
# define USE_MAPBOX_VARIANT 0
#endif // define USE_MAPBOX_VARIANT

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "simdjson/simdjson.h"
#include "gretl4py_types.h"
#include <gretl/libgretl.h>
#include <gretl/matrix_extra.h>
#include <gretl/genmain.h>
#include <gretl/objstack.h>
#include <iostream>
#include <unordered_set>

namespace py = pybind11;

//////////////////////////////////////////////////
// macros
//////////////////////////////////////////////////
#define is_pybool(o) (py::isinstance<py::bool_>(o))
#define is_pyint(o) (py::isinstance<py::int_>(o))
#define is_pyfloat(o) (py::isinstance<py::float_>(o))
#define is_pystr(o) (py::isinstance<py::str>(o))
#define is_pydict(o) (py::isinstance<py::dict>(o))
#define is_pytuple(o) (py::isinstance<py::tuple>(o))
#define is_pynumeric(o) (is_pyint(o) || is_pyfloat(o))


//////////////////////////////////////////////////
// functions
// Note: some signature(s) may be at the bottom of this header
//////////////////////////////////////////////////
void handle_gretl_error (int err);

GretlTypes *gretl_generate_gretl_object (const char *s, int &err,
                                         GretlObjType obj_type = GRETL_OBJ_INFO,
                                         DATASET *dset = NULL,
                                         MODEL *model = NULL);

//
// Class to handle printing
//
class GretlPrint
{
private:
    PRN *prn = NULL;
    PrnType prn_type;
public:
    int err = 0;

    GretlPrint (PrnType prn_type = GRETL_PRINT_BUFFER);  // by default we use buffered printer
    ~GretlPrint (void);

    PRN *prn_ptr (void);

    void print (void);

    const char *get_buffer (void);
};


//
// Class to handle gretl 'DATASET' type
//
class GretlDataset
{
private:
    static inline int next_obj_id = 1;
    int id;
    std::unordered_set<int> linked_models;  // models (id-s) which use this dataset
    GretlPrint GP = GretlPrint();           // printer (instance wide)

    void allocate_dset (void);

    void free_dset (void);

    gretlopt setobs_opt_from_structure (int structure);
public:
    mutable int err = 0;
    bool is_private = false;
    bool is_default = false;
    DATASET *dset = NULL;
    std::string source;

    GretlDataset (const std::string fname); // we load dataset from file
    GretlDataset (const py::dict &pydict);  // we load dataset from Python's dictionary
    ~GretlDataset (void);

    py::dict *to_dict (void) const;         // memory must be deleted at the caller level

    void to_file (const std::string fname) const;

    py::str repr (void) const;

    void print (const py::kwargs &kwargs);

    void sample (const py::kwargs &kwargs);

    void set_as_default (void);

    void setobs (const py::kwargs &kwargs);

    void new_series (const std::string formula);

    py::list *varnames (void) const;         // memory must be deleted at the caller level

    int get_id (void) const;

    void linked_models_add (const int model_id);

    void linked_models_del (const int model_id);

    std::unordered_set<int> linked_models_list (void) const;

    GretlTypes *get_accessor (const std::string &accessor) const;
};

//
// Class to handle 'gretl_bundle' type
//
class GretlBundle
{
private:
    // simdjson global buffers
    static inline simdjson::ondemand::parser sd_parser{};
    static inline simdjson::ondemand::document sd_doc;
    static inline simdjson::ondemand::object sd_obj;
    static inline simdjson::ondemand::value sd_tmp_value;
    static inline simdjson::ondemand::number sd_tmp_number;
    static inline simdjson::ondemand::array sd_tmp_array;
    static inline std::string sd_json_cp; // those two strings we may change to char* buffers
    static inline std::string sd_tmp_str;
    //simdjson: end
    GretlPrint GP = GretlPrint();               // printer (instance wide)
    bool free_b = true;
    char *json_buf = NULL;
    gretl_bundle *b;
    py::dict *as_dict = nullptr;

    py::dict *json_to_dict (simdjson::ondemand::object &json_obj,
                            py::dict *outer_dict = nullptr,
                            std::string_view *outer_key = nullptr,
                            bool *add_to_outer = nullptr);
public:
    int err = 0;

    GretlBundle (void);                         // we get contents of $sysinfo bundle
    GretlBundle (MODEL *model, DATASET *dset);  // we produce bundle from MODEL struct
    GretlBundle (gretl_bundle *b);              // incoming already allocated bundle
    ~GretlBundle (void);

    py::dict *to_dict (void);                   // bundle-to-dict mapper: memory must be deleted at the caller level

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

    inline std::string to_string (void) { return as_str; }

    inline std::string_view to_string_view (void) { return static_cast<std::string_view>(as_str); }
};

// function implemented in gretl_calls.cpp
void set_default_dset (GretlDataset *GD);

#endif // GRETL4PY_DATA_H
