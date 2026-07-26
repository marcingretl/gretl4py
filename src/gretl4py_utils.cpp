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

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "gretl4py_utils.h"
#include "gretl4py_types.h"
#include "gretl4py_data.h"
#include "gretl4py_model.h"
extern "C" {
    #include <gretl/gretl_typemap.h>
}

namespace py = pybind11;

//////////////////////////////////////////////////
// free functions
//////////////////////////////////////////////////
std::string gretl4py_trim (std::string &s)
{
    //
    // general purpose trimer
    //
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());

    return s;
}

std::string gretl4py_to_lower (std::string s)
{
    //
    // convert string to lower case
    //
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c)
                   {
                       return static_cast<char>(std::tolower(c));
                   });

    return s;
}

void NumPyArray_extract (const py::array &arr, NumPyArrayView &view)
{
    //
    // here we extract meta information about NumPy arrays
    //
    if (arr.ndim() > 2) throw std::runtime_error("Given array has more than two dimensions.");

    view.is_double = arr.dtype().is(py::dtype::of<double>());
    view.is_complex = arr.dtype().is(py::dtype::of<std::complex<double>>());
    view.dtype = arr.dtype();
    view.base_array = arr;          // keep the NumPy array alive
    view.data = arr.request().ptr;
    view.rows = arr.shape(0);
    view.cols = arr.ndim() == 1 ? 1 : arr.shape(1);
    view.NumPyArrayFC = is_numpy_array_fc(arr);

    const ssize_t itemsize = view.dtype.itemsize();
    const auto &strides = arr.strides();

    view.row_stride = strides[0] / itemsize;
    view.col_stride = (arr.ndim() == 1) ? 0 : strides[1] / itemsize;
}

// Note: this is needed to avoid memory leaks when
// transferring GretlTypes to Python inside py::dict
py::object variant_to_pyobject (GretlTypes &&val)
{
    return std::visit([](auto &&arg) -> py::object
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_base_of_v<py::object, T>) {
            return std::move(arg);  // steal py::dict, py::list, etc.
        } else {
            return py::cast(std::move(arg));  // move std::string, etc.
        }
    }, std::move(val));
}

template <typename T>
void get_object_from_dict (const py::dict &dict, const char *key, T &target)
{
    //
    // function for getting complex objects (lists, dicts, tuples) from kwargs
    //
    if (!dict.contains(key)) return;

    // lambda for handling exceptions
    auto key_not_a = [key](const char *obj_type)
    {
        throw std::invalid_argument(std::string("'") + key + "' must be a " + obj_type);
    };

    auto obj = dict[key];

    if constexpr (std::is_same_v<T, std::vector<std::string>>) {
        if (!is_pylist(obj)) key_not_a("list");
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (!is_pystr(obj)) key_not_a("string");
    } else if constexpr (std::is_same_v<T, py::array_t<double>>) {
        if (!is_pynumarrayd(obj)) key_not_a("ndarray");
    } else {
        static_assert(always_false<T>, "get_object_from_dict(): unknown type");
    }

    if constexpr (std::is_same_v<T, py::array_t<double>>) {
        target = py::reinterpret_borrow<py::array_t<double>>(obj);
    } else {
        target = py::cast<T>(obj);
    }
}
template void get_object_from_dict<std::vector<std::string>>(const py::dict&, const char*, std::vector<std::string>&);
template void get_object_from_dict<std::string>(const py::dict&, const char*, std::string&);
template void get_object_from_dict<py::array_t<double>>(const py::dict&, const char*, py::array_t<double>&);

GretlTypesPtr convert_GretlType_to_GretlTypes (GretlType t, void *gr_obj_ptr,
                                               const int n, int &err)
{
    //
    // function converts libgretl's GretlType to gretl4py's GretlTypes
    // *gr_obj_ptr: pointer to libgretl's object
    // n - GRETL_TYPE_SERIES special case (series length)
    //
    auto res = std::make_unique<GretlTypes>();

    switch (t) {
    case GRETL_TYPE_NONE: {
        py::print("gretl_grab: didn't get anything!\n");
        *res = py::none();
        break;
    }
    case GRETL_TYPE_VOID: {
        *res = py::none();
        break;
    }
    case GRETL_TYPE_INT: {
        *res = *(static_cast<int*>(gr_obj_ptr));
        break;
    }
    case GRETL_TYPE_DOUBLE:
        *res = *(static_cast<double*>(gr_obj_ptr));
        break;
    case GRETL_TYPE_STRING: {
        *res = static_cast<char*>(gr_obj_ptr);
        break;
    }
    case GRETL_TYPE_MATRIX: {
        *res = std::make_shared<GretlMatrix>(static_cast<gretl_matrix*>(gr_obj_ptr));
        break;
    }
    case GRETL_TYPE_BUNDLE: {
        *res = std::make_shared<GretlBundle>(static_cast<gretl_bundle*>(gr_obj_ptr));
        break;
    }
    case GRETL_TYPE_ARRAY:
    case GRETL_TYPE_MATRICES:
    case GRETL_TYPE_STRINGS:
    case GRETL_TYPE_BUNDLES: {
        gretl_array *a = static_cast<gretl_array*>(gr_obj_ptr);
        py::list py_list;
        GretlType tmp;

        for (int i=0; i<gretl_array_get_length(a); i++) {
            tmp = gretl_array_get_content_type(a);
            auto element = convert_GretlType_to_GretlTypes(tmp, gretl_array_get_element(a, i, &tmp, &err), n, err);
            py_list.append(variant_to_pyobject(std::move(*element)));
        }
        *res = py_list;
        break;
    }
    case GRETL_TYPE_SERIES: {
        DNumPyArrayFC mat(n);

        std::memcpy(mat.mutable_data(), static_cast<double*>(gr_obj_ptr), n * sizeof(double));
        *res = mat;
        break;
    }
    case GRETL_TYPE_LIST: {
        int *l = static_cast<int*>(gr_obj_ptr);
        py::list py_list;

        for (int i=1; i<=l[0]; ++i) {
            py_list.append(l[i]);
        }
        *res = py_list;
        break;
    }
    default:
        py::print("gretl_grab: got unsupported type:", gretl_type_get_name(t));
        *res = py::none();
    }

    return res;
}

PythonArgInfo inspect_pyobject (py::handle obj)
{
    //
    // here we check what kind of argument arrived from python
    // and produce a struct with meta information
    //
    PythonArgInfo info;
    info.obj = obj;

    if (obj.is_none()) {
        info.gtype = GRETL_TYPE_NONE;
        info.pytype = "NoneType";
        return info;
    }
    if (is_pybool(obj)) {
        info.gtype = GRETL_TYPE_BOOL;
        info.pytype = "bool";
        return info;
    }
    if (is_pyint(obj)) {
        info.gtype = GRETL_TYPE_INT;
        info.pytype = "int";
        return info;
    }
    if (is_pyfloat(obj)) {
        info.gtype = GRETL_TYPE_DOUBLE;
        info.pytype = "float";
        return info;
    }
    if (is_pystr(obj)) {
        info.gtype = GRETL_TYPE_STRING;
        info.pytype = "str";
        return info;
    }
    if (is_pygretlbundle(obj)) {
        info.gtype = GRETL_TYPE_BUNDLE;
        info.pytype = "GretlBundle";
        return info;
    }
    if (is_pygretlmatrix(obj)) {
        info.gtype = GRETL_TYPE_MATRIX;
        info.pytype = "GretlMatrix";
        return info;
    }
    if (is_pynumarray(obj)) {
        info.gtype = GRETL_TYPE_MATRIX;
        info.pytype = "ndarray";
        auto arr = py::cast<py::array>(obj);
        info.shape.assign(arr.shape(), arr.shape() + arr.ndim());
        return info;
    }
    if (is_pytuple(obj) || is_pylist(obj)) {
        info.gtype = GRETL_TYPE_LIST;
        info.pytype = py::isinstance<py::tuple>(obj) ? "tuple" : "list";
        auto seq = py::reinterpret_borrow<py::sequence>(obj);
        info.shape.push_back(seq.size());
        return info;
    }
    info.gtype = GRETL_TYPE_ANY;
#if PYBIND11_VERSION_MAJOR < 3
    info.pytype = py::str(obj.get_type());
#else
    info.pytype = py::str(py::type::of(obj));
#endif
    info.valid = false;

    return info;
}

// Here we move item to named container for given instance
template <typename PyWrapped, typename Item, typename ContainerType>
void move_to_python_container (PyWrapped &wrapped_obj, Item &&item,
                               const char *container, const std::string &key)
{
    py::gil_scoped_acquire gil;
    py::object obj = py::cast(wrapped_obj);

    // we check if given container exists, and if not we create it
    if (!py::hasattr(obj, container)) {
        if constexpr (std::is_same_v<ContainerType, py::list>) {
            obj.attr(container) = py::list();
        } else if constexpr (std::is_same_v<ContainerType, py::dict>) {
            obj.attr(container) = py::dict();
        } else {
            static_assert(always_false<ContainerType>, "move_to_python_container(): unknown Container type");
        }
    }
    py::object dst = obj.attr(container);

    // we move an item to the container
    if constexpr (std::is_same_v<ContainerType, py::list>) {
        if (!is_pylist(dst)) throw std::runtime_error("attribute '" + std::string(container) + "' is not a list");
        dst.cast<py::list>().append(std::forward<Item>(item));
    } else if constexpr (std::is_same_v<ContainerType, py::dict>) {
        if (!is_pydict(dst)) throw std::runtime_error("attribute '" + std::string(container) + "' is not a dictionary");
        dst.cast<py::dict>()[py::cast(key)] = std::forward<Item>(item);
    } else {
        static_assert(always_false<ContainerType>, "move_to_python_container(): unknown Container type");
    }
}
template void move_to_python_container<GretlModel, py::dict, py::list>(GretlModel&, py::dict&&, const char*, const std::string&);
template void move_to_python_container<GretlDataset, py::dict, py::list>(GretlDataset&, py::dict&&, const char*, const std::string&);

template <typename T>
void load_string_to_container (T &t, const std::string &src)
{
    //
    // we extract space-separated string and load to container
    //
    std::istringstream iss(src);
    std::string token;

    while (iss >> token) {
        if constexpr (std::is_same_v<T, std::unordered_multiset<std::string>>) {
            t.insert(token);
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            t.push_back(token);
        } else {
            static_assert(always_false<T>, "load_string_to_container(): unknown Container type");
        }
    }
}
template void load_string_to_container<std::unordered_multiset<std::string>>(std::unordered_multiset<std::string>&, const std::string&);
template void load_string_to_container<std::vector<std::string>>(std::vector<std::string>&, const std::string&);

bool is_bool_kwarg (const py::handle &h, const std::string &key)
{
    //
    // we handle 0/1/True/False/"0"/"1"/etc in a robust way
    //
    if (is_pybool(h)) return h.cast<bool>();
    if (is_pyint(h) || is_pyfloat(h)) {
        const double x = h.cast<double>();

        if (x == 0.0) return false;
        if (x == 1.0) return true;
        handle_gretl_error(E_INVARG, key);
    }
    if (is_pystr(h)) {
        std::string s = py::str(h);

        if (s == "1" || s == "1.0" || s == "true" || s == "True") return true;
        if (s == "0" || s == "0.0" || s == "false" || s == "False") return false;
        handle_gretl_error(E_INVARG, key);
    }
    handle_gretl_error(E_INVARG, key);

    return false;
}

template <typename T>
T c_style_char_array_to_container (c_str_array src, int size)
{
    //
    // convenient function which extract char** to c++/python container
    // note: this function should be called via adapters put below
    //
    if (size <= 0 || src == nullptr) throw std::runtime_error("invalid char array size");
    std::vector<std::string> dst;

    dst.reserve(size);
    for (int i=0; i<size; ++i) {
        src[i] ? dst.emplace_back(src[i]) :dst.emplace_back();
    }

    if constexpr (std::is_same_v<T, std::vector<std::string>>) {
        return dst;                                                 //NRVO
    } else if constexpr (std::is_same_v<T, py::list>) {
        return py::cast(dst);
    } else {
        static_assert(!sizeof(T), "Unsupported container type in c_style_char_array_to_container");
    }
}

std::string get_pyobject_type (const py::handle &obj)
{
    //
    // we return Python's type of @obj
    //
    return py::str(py::type::of(obj).attr("__name__")).cast<std::string>();
}

template <typename T>
MatrixData<T> pylist_to_matrix_data (const py::sequence &seq)
{
    //
    // here we convert sequence to STL 2D vector
    //
    if (seq.empty()) return {};
    py::handle first = seq[0];

    // 1D sequence note: 1D vector is automatically converted to MatrixData
    if (!is_pysequence(first) || is_pystr(first) || is_pybytes(first)) return { seq.cast<std::vector<T>>() };

    // 2D sequence
    return seq.cast<MatrixData<T>>();
}

// adapters to c_style_char_array_to_container()
// this is because const char** and char** are treated by C++ compiler in a different way
template <typename T>
T c_style_char_array_to_container (char **src, int size)
{
    return c_style_char_array_to_container<T>(const_cast<c_str_array>(src), size);
}
template py::list c_style_char_array_to_container<py::list>(char**, int);

template <typename T>
T c_style_char_array_to_container (const char **src, int size)
{
    return c_style_char_array_to_container<T>(reinterpret_cast<c_str_array>(src), size);
}
template py::list c_style_char_array_to_container<py::list>(const char**, int);
