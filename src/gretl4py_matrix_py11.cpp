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

#include "gretl4py_matrix.h"
#include "gretl4py_utils.h"
#include "docstrings/gretl4py_matrix_docstr.h"

namespace py = pybind11;

namespace {
template <bool ROWS>
py::object matrix_row_cols_names_get (const GretlMatrix &m, std::optional<int> index)
{
    //
    // helper for retrieving matrix row/column names.
    //
    auto names = m.rows_cols_names_get<ROWS>();

    if (!index) return std::move(names);
    int i = *index;

    if (i < 0) throw py::index_error("list index out of range");

    return py::str(names[i]);
}

template <typename T>
py::array_t<T> make_numpy_view_impl (const GretlMatrix &m, py::handle owner, T *ptr)
{
    //
    // helper for making NumPy view of GretlMatrix
    //
    if (m.is_vector()) {
        ssize_t n = std::max(m.rows(), m.cols());

        return py::array_t<T>({n}, {sizeof(T)}, ptr, owner);
    }

    return py::array_t<T>({m.rows(), m.cols()}, {sizeof(T), sizeof(T) * m.rows()}, ptr, owner);
}
}

//
// GretlMatrix mappings
//
void init_gretl4py_gretl_matrix_py11 (py::module_ &m)
{
    auto cls = py::class_<GretlMatrix, GretlMatrixPtr>(m, "Matrix", py::dynamic_attr());

    cls
    .def(py::init<>())                                                              // default constructor
    .def(py::init<int, int, double, const std::string&>(),
         py::arg("rows").none(false), py::arg("cols").none(false),
         py::arg("fill").none(false) = 0.0, py::arg("type").none(false) = "real")   // filled real/complex matrix
    .def(py::init<int, int, const std::string&>(),
         py::arg("rows").none(false), py::arg("cols").none(false),
         py::arg("dist").none(false) = "normal")                                    // random matrix
    .def(py::init<double, double, double>(),
         py::arg("a").none(false), py::arg("b").none(false),
         py::arg("k").none(false) = 1.0)                                            // sequential row vector
    .def(py::init<int>(), py::arg("n").none(false))                                 // identity matrix
    .def(py::init<const py::array&>(), py::arg("array").none(false))                // from NumPy array
    .def(py::init<const py::sequence&>(), py::arg("seq").none(false))               // from Python sequence
    .def_static("eye", &GretlMatrix::identity,
                py::arg("n").none(false), py::arg("m") = std::nullopt)
    .def_static("diag", &GretlMatrix::diag, py::arg("seq").none(false))
    .def_static("zeros", &GretlMatrix::zeros,
                py::arg("rows").none(false), py::arg("cols").none(false))
    .def_property_readonly("rows",
                           [](const GretlMatrix &self)
                           {
                               return self.rows();
                           })
    .def_property_readonly("cols",
                           [](const GretlMatrix &self)
                           {
                               return self.cols();
                           })
    .def_property_readonly("is_complex",
                           [](const GretlMatrix &self)
                           {
                               return self.is_complex();
                           })
    .def_property_readonly("is_vector",
                           [](const GretlMatrix &self)
                           {
                               return self.is_vector();
                           })
    .def_property_readonly("numpy_view",
                           [](GretlMatrix &self) -> py::array
                           {
                               if (self.is_complex()) {
                                   return make_numpy_view_impl<std::complex<double>>(self,
                                                                                     py::cast(&self, py::return_value_policy::reference),
                                                                                     reinterpret_cast<std::complex<double>*>(self.z()));
                               }

                               return make_numpy_view_impl<double>(self, py::cast(&self, py::return_value_policy::reference), self.val());
                           })
    .def("__len__", &GretlMatrix::rows)
    .def("__array__",
         [](py::object self_obj, py::object , py::object ) -> py::array
         {
             //
             // to follow the protocol convention in 100% (dtype, copy)
             //
             auto &self = self_obj.cast<GretlMatrix&>();

             if (self.is_complex()) {
                 return make_numpy_view_impl<std::complex<double>>(self, py::cast(&self, py::return_value_policy::reference),
                                                                   reinterpret_cast<std::complex<double>*>(self.z()));
             }

             return make_numpy_view_impl<double>(self, py::cast(&self, py::return_value_policy::reference), self.val());
         }, py::arg("dtype") = py::none(), py::arg("copy") = py::none())
    .def("__copy__", [](const GretlMatrix &self) { return GretlMatrix(self); })
    .def("__deepcopy__", [](const GretlMatrix &self, py::dict) { return GretlMatrix(self); })
    .def("__repr__", &GretlMatrix::repr_impl)
    .def("__getitem__",
         [](const GretlMatrix &self, const py::object &idx) -> py::object
         {
             //
             // we dispatch slicing type
             //
             auto res = std::make_unique<GretlTypes>();

             if (is_pytuple(idx)) {
                 auto tuple = idx.cast<py::tuple>();

                 if (tuple.size() == 2 && (is_pyslice(tuple[0]) || is_pyslice(tuple[1]))) return variant_to_pyobject(self.get_slice(tuple));
                 if (tuple.size() == 2) return self.get_element(tuple[0].cast<int>(), tuple[1].cast<int>());
                 throw std::invalid_argument("Invalid matrix index");
             }
             if (is_pyint(idx)) return self.get_element(idx.cast<int>());
             if (is_pyslice(idx)) return variant_to_pyobject(self.get_slice(idx.cast<py::slice>()));
             //if (is_pystr(idx)) return variant_to_pyobject(self.get_slice(idx.cast<std::string>())); # slicing by string is not allowed for the time
             throw std::invalid_argument("Unsupported index type");
         })
    .def("__setitem__",
         [](GretlMatrix &self, std::pair<int, int> ij, double value) -> void
         {
             self.set_element<double>(ij.first, ij.second, value);
         }, py::arg("ij").none(false), py::arg("value").none(false))
    .def("__setitem__", py::overload_cast<int, double>(&GretlMatrix::set_element<double>),
         py::arg("i").none(false), py::arg("value").none(false))
    .def("__setitem__",
         [](GretlMatrix &self, std::pair<int, int> ij, std::complex<double> value) -> void
         {
             self.set_element<std::complex<double>>(ij.first, ij.second, value);
         }, py::arg("ij").none(false), py::arg("value").none(false))
    .def("__setitem__", py::overload_cast<int, std::complex<double>>(&GretlMatrix::set_element<std::complex<double>>),
         py::arg("i").none(false), py::arg("value").none(false))
    .def("__pos__", [](const GretlMatrix &self) { return GretlMatrix(self); })
    .def("__neg__", &GretlMatrix::neg)
    .def("__eq__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::EQ>)
    .def("__eq__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::EQ>)
    .def("__eq__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::EQ>)
    .def("__eq__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::EQ>)
    .def("__ne__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::NE>)
    .def("__ne__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::NE>)
    .def("__ne__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::NE>)
    .def("__ne__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::NE>)
    .def("__lt__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::LT>)
    .def("__lt__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::LT>)
    .def("__lt__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::LT>)
    .def("__lt__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::LT>)
    .def("__le__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::LE>)
    .def("__le__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::LE>)
    .def("__le__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::LE>)
    .def("__le__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::LE>)
    .def("__gt__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::GT>)
    .def("__gt__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::GT>)
    .def("__gt__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::GT>)
    .def("__gt__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::GT>)
    .def("__ge__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::GE>)
    .def("__ge__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::GE>)
    .def("__ge__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::GE>)
    .def("__ge__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::GE>)
    .def("__add__", &GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::ADD>)
    .def("__add__", &GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::ADD>)
    .def("__add__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::ADD>)
    .def("__add__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::ADD>)
    .def("__sub__", &GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::SUB>)
    .def("__sub__", &GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::SUB>)
    .def("__sub__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::SUB>)
    .def("__sub__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::SUB>)
    .def("__mul__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::MUL>)
    .def("__mul__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::MUL>)
    .def("__mul__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::MUL>)
    .def("__mul__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::MUL>)
    .def("__matmul__", &GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::MATMUL>)
    .def("__matmul__", &GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::MATMUL>)
    .def("__truediv__", &GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::TRUEDIV>)
    .def("__truediv__", &GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::TRUEDIV>)
    .def("__truediv__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::TRUEDIV>)
    .def("__truediv__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::TRUEDIV>)
    .def("__pow__", &GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::POW>)
    .def("__pow__", &GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::POW>)
    .def("__pow__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::POW>)
    .def("__pow__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::POW>)
    .def("__iadd__",
         [](GretlMatrix &self, const GretlMatrix &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::IADD>(rhs);
             } else {
                 return self.alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::IADD>(rhs);
             }
         }, py::arg("mat").none(false))
    .def("__iadd__",
         [](GretlMatrix &self, const py::array &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::IADD>(rhs);
             } else {
                 return self.alg_inplace_operations<py::array, GretlMatrixAlgOperations::IADD>(rhs);
             }
         }, py::arg("mat").none(false))
    .def("__iadd__", &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IADD>)
    .def("__iadd__", &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IADD>)
    .def("__isub__",
         [](GretlMatrix &self, const GretlMatrix &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::ISUB>(rhs);
             } else {
                 return self.alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::ISUB>(rhs);
             }
         }, py::arg("mat").none(false))
    .def("__isub__",
         [](GretlMatrix &self, const py::array &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::ISUB>(rhs);
             } else {
                 return self.alg_inplace_operations<py::array, GretlMatrixAlgOperations::ISUB>(rhs);
             }
         }, py::arg("mat").none(false))
    .def("__isub__", &GretlMatrix::alg_inplace_operations<py::array, GretlMatrixAlgOperations::ISUB>)
    .def("__isub__", &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::ISUB>)
    .def("__isub__", &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::ISUB>)
    .def("__imul__", &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IMUL>)
    .def("__imul__", &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IMUL>)
    .def("__itruediv__", &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::ITRUEDIV>)
    .def("__itruediv__", &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::ITRUEDIV>)
    .def("__ipow__",
         [](GretlMatrix &self, const GretlMatrix &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::IPOW>(rhs);
             } else {
                 return self.alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::IPOW>(rhs);

             }
         }, py::arg("mat").none(false))
    .def("__ipow__",
         [](GretlMatrix &self, const py::array &rhs) -> GretlMatrix&
         {
             if (self.is_complex()) {
                 return self.alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::IPOW>(rhs);
             } else {
                 return self.alg_inplace_operations<py::array, GretlMatrixAlgOperations::IPOW>(rhs);
             }
         }, py::arg("mat").none(false))
    .def("__ipow__", &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IPOW>)
    .def("__ipow__", &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IPOW>)
    .def("__radd__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::ADD>)
    .def("__radd__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::ADD>)
    .def("__rsub__",
         [](const GretlMatrix &self, double val) -> GretlMatrixPtr
         {
             auto res = self.neg();

             res->alg_inplace_operations<double, GretlMatrixAlgOperations::IADD>(val);

             return res;
         })
    .def("__rsub__",
         [](const GretlMatrix &self, std::complex<double> val) -> GretlMatrixPtr
         {
             return self.neg()->alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::ADD>(val);
         })
    .def("__rmul__", &GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::MUL>)
    .def("__rmul__", &GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::MUL>)
    .def("__rtruediv__",
         [](const GretlMatrix &self, double val) -> GretlMatrixPtr
         {
             auto res = self.alg_dot_operations<double, GretlMatrixAlgOperations::TRUEDIV>(1.0);

             res->alg_inplace_operations<double, GretlMatrixAlgOperations::IMUL>(val);

             return res;
         })
    .def("__rtruediv__",
         [](const GretlMatrix &self, std::complex<double> val) -> GretlMatrixPtr
         {
             auto res = self.alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::TRUEDIV>(1.0);

             return res->alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::MUL>(val);
         })
    .def("copy", [](const GretlMatrix &self) { return GretlMatrix(self); })
    .def("to_numpy", &GretlMatrix::to_numpy)
    .def("rnameget", &matrix_row_cols_names_get<true>, py::arg("row").none(true) = std::nullopt)
    .def("cnameget", &matrix_row_cols_names_get<false>, py::arg("col").none(true) = std::nullopt)
    .def("rnameset", &GretlMatrix::rows_cols_names_set<true>, py::arg("names").none(false))
    .def("cnameset", &GretlMatrix::rows_cols_names_set<false>, py::arg("names").none(false))
    .def("summary", &GretlMatrix::summary, doc::Matrix::summary);

    cls.attr("__module__") = "gretl";
}
