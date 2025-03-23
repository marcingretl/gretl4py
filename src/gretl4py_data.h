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

#include "simdjson/simdjson.h"
#include <pybind11/stl.h>
#include <gretl/libgretl.h>
#include <iostream>

namespace py = pybind11;

//////////////////////////////////////////////////
// types
//////////////////////////////////////////////////
#if USE_MAPBOX_VARIANT
# include "mapbox/variant.hpp"
 typedef std::vector<mapbox::util::variant<double, std::string, int>> vgretl_vector;
#else
 typedef std::vector<std::variant<double, std::string, int>> vgretl_vector;
#endif // USE_MAPBOX_VARIANT
typedef std::vector<vgretl_vector> vgretl_matrix;

//////////////////////////////////////////////////
// macros
//////////////////////////////////////////////////
#define is_pybool(o) (py::isinstance<py::bool_>(o))
#define is_pyint(o) (py::isinstance<py::int_>(o))
#define is_pyfloat(o) (py::isinstance<py::float_>(o))
#define is_pystr(o) (py::isinstance<py::str>(o))
#define is_pydict(o) (py::isinstance<py::dict>(o))
#define is_pytuple(o) (py::isinstance<py::tuple>(o))

//////////////////////////////////////////////////
// pybind11 extensions
//////////////////////////////////////////////////
#if USE_MAPBOX_VARIANT
namespace PYBIND11_NAMESPACE
{
    namespace detail
    {
        template <typename... Ts>
        struct type_caster<mapbox::util::variant<Ts...>> : variant_caster<mapbox::util::variant<Ts...>> {};

        template <>
        struct visit_helper<mapbox::util::variant> {
            template <typename... Args>
            static auto call(Args &&...args) -> decltype(mapbox::util::apply_visitor(args...))
            {
                return mapbox::util::apply_visitor(args...);
            }
        };
    }
}
#endif // USE_MAPBOX_VARIANT

py::dict *gretl_model_to_dict (MODEL *model, DATASET *dset,
                               const char *summary, int *err);

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

    py::dict *to_dict (void);               // memory must be deleted at the caller level

    void printdset (bool namesonly = false);

    void sample (const py::kwargs &kwargs);
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

    //auto *ptr (void) { return res; } // not used

    inline std::string to_string (void) { return as_str; }

    inline std::string_view to_string_view (void) { return static_cast<std::string_view>(as_str); }
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

#endif // GRETL4PY_DATA_H
