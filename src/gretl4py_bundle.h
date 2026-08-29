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

#ifndef GRETL4PY_BUNDLE_H
#define GRETL4PY_BUNDLE_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "gretl4py_types.h"
#include "gretl_calls.h"
#include <gretl/gretl_matrix.h>
extern "C" {
    #include <gretl/gretl_func.h>
}

namespace py = pybind11;

//
// Class to handle 'gretl_bundle' type
//
class GretlBundle
{
private:
    bool free_b_ = true;                            // if *b should be freed
    gretl_bundle *b_ = nullptr;                     // libgretl bundle pointer
    py::dict as_dict_;                              // Python's dict() representation
    int err_ = 0;

    template <typename F>
    void for_each_bundle_element_ (F &&f);

    void extract_ (const py::object &obj);          // to register as given obj

    void extract_ (py::dict &dict);                 // to convert to dict
public:
    GretlBundle (void);                                 // default constructor with empty bundle
    GretlBundle (const GretlBundle &other);             // copy constructor
    GretlBundle (gretl_bundle *src, bool copy = true);  // if copy==false, incoming bundle must be deallocated by libgretl
    GretlBundle (MODEL *model, DATASET *dset);          // we produce bundle from MODEL
    GretlBundle (GRETL_VAR *model, DATASET *dset);      // we produce bundle from VAR/VECM
    GretlBundle (equation_system *model);               // we produce bundle from SYSTEM
    GretlBundle (const py::kwargs &kwargs);             // for on-the-fly initialization
    ~GretlBundle (void);

    GretlBundle &operator= (const GretlBundle &other);

    int get_err (void) const;

    py::dict to_dict (void);                        // bundle-to-dict mapper

    void python_register (py::object &obj);         // register bundle contents in python object

    std::string repr_impl (void);                   // bundle printing via __repr__

    gretl_bundle *get_ptr (void) const;             // raw gretl_bundle pointer

    py::list keys (void);

    template <typename T>
    T print (bool tree = false);                    // bundle printer

    template <typename T>
    T get_element (const std::string &key);         // get bundle element by name

    template <typename T>
    void set_element (const std::string &key, const T &t);

    void delete_element (const std::string &key);

    py::object pop (const std::string &key);
};

#endif // GRETL4PY_BUNDLE_H
