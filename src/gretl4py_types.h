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

#ifndef GRETL4PY_TYPES_H
#define GRETL4PY_TYPES_H

// Ensure pybind11 and libgretl are included wherever this header is used
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <gretl/libgretl.h>
#include <gretl/gretl_drivers.h>
#include <memory>
#include <iostream>

// Workaround for gettext redefinition
#undef gettext

namespace py = pybind11;

//////////////////////////////////////////////////
// smart pointers
//////////////////////////////////////////////////

struct GretlListDeleter {
    void operator() (int *ptr) const
    {
        #if defined(_WIN32)
            gretl_list_free(ptr);
        #else
            std::free(ptr);
        #endif
    }
};
using gretl_list_ptr = std::unique_ptr<int, GretlListDeleter>;

struct GretlDoubleDeleter {
    void operator() (double *ptr) const
    {
        std::free(ptr);
    }
};
using gretl_double_ptr = std::unique_ptr<double, GretlDoubleDeleter>;

struct GretlMatrixDeleter {
    void operator() (gretl_matrix *ptr) const
    {
        gretl_matrix_free(ptr);
    }
};
using gretl_matrix_ptr = std::unique_ptr<gretl_matrix, GretlMatrixDeleter>;

struct GretlGcharDeleter {
    void operator() (gchar *ptr) const
    {
        g_free(ptr);
    }
};
using gretl_gchar_ptr = std::unique_ptr<gchar, GretlGcharDeleter>;

// function results deleter
struct GretlResourceDeleter {
    GretlType type = GRETL_TYPE_NONE;

    void operator() (void *ptr) const {
        if (!ptr) return;

        switch (type) {
            case GRETL_TYPE_MATRIX:
                gretl_matrix_free(static_cast<gretl_matrix*>(ptr));
                break;
            case GRETL_TYPE_BUNDLE:
                gretl_bundle_destroy(static_cast<gretl_bundle*>(ptr));
                break;
            case GRETL_TYPE_STRING:
                std::free(static_cast<char*>(ptr));
                break;
            case GRETL_TYPE_SERIES:
                std::free(static_cast<double*>(ptr));
                break;
            case GRETL_TYPE_MATRICES:
            case GRETL_TYPE_STRINGS:
            case GRETL_TYPE_BUNDLES:
                gretl_array_destroy(static_cast<gretl_array*>(ptr));
                break;
            case GRETL_TYPE_DOUBLE:
            case GRETL_TYPE_LIST:
                // not owned here
                break;
            default:
                std::cerr << "GretlResourceDeleter: unsupported type" << std::endl;
        }
    }
};
using GretlResourcePtr = std::unique_ptr<void, GretlResourceDeleter>;

// struct for holding results returned by functions from contributed packages
struct GretlPackageRes {
    void *raw_ptr = nullptr;            // pointer to returned value
    GretlType type = GRETL_TYPE_NONE;   // type of returned object
    int n = 0;                          // GRETL_TYPE_SERIES special case: length

    void set_type (GretlType t) { type = t; }

    void *get (void) const { return raw_ptr; }

    void **out_ptr (void) { return &raw_ptr; }

    void transfer_ownership (void) {
        ptr = GretlResourcePtr(raw_ptr, GretlResourceDeleter{type});
    }

private:
    GretlResourcePtr ptr = {nullptr, GretlResourceDeleter{GRETL_TYPE_NONE}};
};

// gretl4py classes
class GretlDataset;
using GretlDatasetPtr = std::shared_ptr<GretlDataset>;

class GretlBundle;
using GretlBundlePtr = std::shared_ptr<GretlBundle>;

class GretlModel;
using GretlModelPtr = std::unique_ptr<GretlModel>;

class GretlPackage;
using GretlPackagePtr = std::shared_ptr<GretlPackage>;


//////////////////////////////////////////////////
// types
//////////////////////////////////////////////////
using DNumPyArrayFC = py::array_t<double, py::array::f_style>;  // double contiguous column-major (fortran) NumPy array (F_CONTIGUOUS)

using GretlTypes = std::variant<int, double, std::string, DNumPyArrayFC,
                                py::dict, py::list, GretlBundlePtr,
                                py::none>;

using GretlTypesPtr = std::unique_ptr<GretlTypes>;

using vgretl_vector = std::vector<std::variant<double, std::string, int>>;

using vgretl_matrix = std::vector<vgretl_vector>;

struct NumPyArrayView {
    void *data = nullptr;   // untyped pointer
    py::dtype dtype;        // numpy dtype info
    py::array base_array;   // holds ownership
    bool is_double = false; // for fast path
    bool NumPyArrayFC = false;
    int rows = 0;
    int cols = 0;
    // note strides in elements (not bytes!)
    ssize_t row_stride = 0;
    ssize_t col_stride = 0;
};

struct PythonArgInfo {
    py::handle obj;             // borrowed reference
    GretlType gtype;
    std::string pytype;         // human readable name of type
    std::vector<ssize_t> shape; // for arrays/lists/tuples
    bool valid = true;          // if we deal with valid object (in terms of gretl4py)
};

#endif // GRETL4PY_TYPES_H
