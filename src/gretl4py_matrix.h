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

#ifndef GRETL4PY_MATRIX_H
#define GRETL4PY_MATRIX_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "gretl4py_types.h"
#include "gretl_calls.h"
#include <gretl/gretl_matrix.h>

namespace py = pybind11;

//
// Class to handle 'gretl_matrix' type
//
class GretlMatrix
{
private:
    gretl_matrix *m_ = nullptr;                                             // libgretl gretl_matrix raw pointer
    mutable GretlPrint printer_ = GretlPrint();                             // printer (instance wide)
    mutable int err_ = 0;

    void check_bounds_ (int i, int j) const;                                // we check if given [i,j] are valid

    void check_bounds_ (int i) const;                                       // we check if given [i] is valid (vector case)

    static gretl_matrix *numpy_to_matrix_ (const py::array &array);         // helper for NumPy to GretlMatrix

    gretl_matrix_ptr exec_slice_ (const std::string &slice_str) const;      // helper for getting slice

    std::string render_slice_ (const py::tuple &slice) const;               // tuple slice to libgretl string slice

    std::string render_slice_ (const py::slice &slice) const;               // slice slice to libgretl string slice

    std::string render_slice_ (const std::string &slice) const;             // string slice to libgretl string slice

    template<typename T>
    void from_matrix_ (const MatrixData<T> &data);                          // to get source data from STL vector
public:
    GretlMatrix (void);                                                             // default constructor
    GretlMatrix (const GretlMatrix &src);                                           // copy constructor
    GretlMatrix (int rows, int cols, double fill, const std::string &type);         // filled real/complex matrix
    GretlMatrix (int rows, int cols, const std::string &dist);                      // random matrix
    GretlMatrix (double start, double end, double step);                            // sequential row vector
    GretlMatrix (int n);                                                            // identity matrix
    GretlMatrix (const py::array &array);                                           // NumPy to GretlMatrix
    GretlMatrix (gretl_matrix *src, bool copy = true);                              // gretl_matrix to GretlMatrix
    GretlMatrix (const py::sequence &seq);                                          // Python sequence to GretlMatrix
    GretlMatrix (const double *src, int n, GretlMatrixMod mod = GRETL_MOD_NONE);    // from vector of doubles
    GretlMatrix (const double **src, int rows, int cols);                           // from array of doubles
    ~GretlMatrix (void);

    GretlMatrix &operator= (const GretlMatrix &src);                        // copy assignment

    static GretlMatrixPtr identity (int n, std::optional<int>);             // static method for creating an I(n,m) like indentity

    static GretlMatrixPtr diag (const py::sequence &seq);                   // static method for creating an I(n) with main diag filled with @seq

    static GretlMatrixPtr zeros (int rows, int cols);                       // static method for creating matrix filled with zeros

    gretl_matrix *get_ptr (void) const;                                     // raw pointer to m_

    double *val (void) const;                                               // raw pointer to double storage

    double _Complex *z (void) const;                                        // raw pointer to complex storage

    int rows (void) const;

    int cols (void) const;

    [[nodiscard]] bool is_complex (void) const noexcept;

    [[nodiscard]] bool is_vector (void) const noexcept;

    std::string repr_impl (void);                                           // gretl_matrix printing via __repr__

    template <bool ROWS>
    py::list rows_cols_names_get (void) const;

    template <bool ROWS>
    void rows_cols_names_set (const py::list &src) const;

    void set_diagonal (const gretl_matrix *src);                            // set diagonal elements from @src

    py::object get_element (int i, int j) const;                            // get element by standard [i,j] indexing

    py::object get_element (int i) const;                                   // get element by [i]: special case for vectors

    GretlMatrixPtr get_slice (const py::tuple &slice) const;

    GretlMatrixPtr get_slice (const py::slice &slice) const;

    GretlMatrixPtr get_slice (const std::string &slice) const;

    template <typename T>
    void set_element (int i, int j, T val);                                 // set [i,j] element of a matrix

    template <typename T>
    void set_element (int i, T val);                                        // set [i] element of a vector

    py::object to_numpy (void) const;

    py::object summary (const py::kwargs &kwargs);

    // matrix-algebra methods
    GretlMatrixPtr neg (void) const;

    template <typename T, GretlMatrixAlgOperations KIND>
    GretlMatrixPtr alg_operations (const T &rhs) const;

    template <typename T, GretlMatrixAlgOperations KIND>
    GretlMatrix &alg_inplace_operations (const T &rhs);

    template <typename T, GretlMatrixAlgOperations KIND>
    GretlMatrix &alg_inplace_operations_complex (const T &rhs);

    template <typename T, GretlMatrixAlgOperations KIND>
    GretlMatrixPtr alg_dot_operations (const T &rhs) const;
};

#endif // GRETL4PY_MATRIX_H
