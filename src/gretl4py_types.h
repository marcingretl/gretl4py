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

#include <pybind11/stl.h>

namespace py = pybind11;

//////////////////////////////////////////////////
// types
//////////////////////////////////////////////////
using DNumPyArrayFC = py::array_t<double, py::array::f_style>; //double contiguous column-major (fortran) NumPy array (F_CONTIGUOUS)

#if USE_MAPBOX_VARIANT
#include "mapbox/variant.hpp"
 using vgretl_vector = std::vector<mapbox::util::variant<double, std::string, int>>;
 using GretlTypes = mapbox::util::variant<double, std::string, DNumPyArrayFC, py::dict, py::list, py::none>;
#else
 using vgretl_vector = std::vector<std::variant<double, std::string, int>>;
 using GretlTypes = std::variant<double, std::string, DNumPyArrayFC, py::dict, py::list, py::none>;
#endif // USE_MAPBOX_VARIANT

using vgretl_matrix = std::vector<vgretl_vector>;

using gretl_list_ptr = std::unique_ptr<int, void(*)(int*)>;

// custom libgretl's list deleter
static inline void gretl_list_deleter (int *ptr) {
#if defined(_WIN32)
    gretl_list_free(ptr);
#else
    std::free(ptr);
#endif
}

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

#endif // GRETL4PY_TYPES_H
