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
#include "gretl4py_bundle.h"
#include "gretl4py_utils.h"
extern "C" {
    #include <gretl/var.h>
    #include <gretl/system.h>
}

//
// GretlBundle class methods
//
GretlBundle::GretlBundle (void) : b_(gretl_bundle_new())
{
    if (!b_) handle_gretl_error(E_ALLOC);
}

GretlBundle::GretlBundle (MODEL *model, DATASET *dset)
{
    b_ = bundle_from_model(model, dset, &err_);
    if (err_) handle_gretl_error(err_);
}

GretlBundle::GretlBundle (GRETL_VAR *model, DATASET *dset) : b_(gretl_bundle_new())
{
    if (!b_) handle_gretl_error(E_ALLOC);
    err_ = gretl_VAR_bundlize(model, dset, b_);
    if (err_) handle_gretl_error(err_);
}

GretlBundle::GretlBundle (equation_system *model) : b_(gretl_bundle_new())
{
    if (!b_) handle_gretl_error(E_ALLOC);
    err_ = equation_system_bundlize(model, b_);
    if (err_) handle_gretl_error(err_);
}

GretlBundle::GretlBundle (gretl_bundle *src, bool copy)
{
    //
    // Here we either steal or make a copy of src
    // Note: if copy, 'src' _must_ be deallocated by libgretl
    //
    if (copy) {
        b_ = gretl_bundle_copy(src, &err_);
        free_b_ = false;
    } else {
        b_ = src;
    }
    if (err_) handle_gretl_error(err_);
}

GretlBundle::GretlBundle (const py::kwargs &kwargs) : b_(gretl_bundle_new())
{
    //
    // this is for mimics libgretl's _() bundle initializer
    //
    if (!b_) handle_gretl_error(E_ALLOC);

    // we iterate over kwargs
    for (const auto &[key, val] : kwargs) {
        auto key_str = key.cast<std::string>();

        if (is_pybool(val)) {
            set_element<bool>(key_str, val.cast<bool>());
        } else if (is_pyint(val)) {
            set_element<int>(key_str, val.cast<int>());
        } else if (is_pyfloat(val)) {
            set_element<double>(key_str, val.cast<double>());
        } else if (is_pystr(val)) {
            set_element<std::string>(key_str, val.cast<std::string>());
        } else if (is_pynumarray(val)) {
            set_element<py::array>(key_str, val.cast<py::array>());
        } else if (py::isinstance<GretlBundle>(val)) {
            set_element<GretlBundlePtr>(key_str, val.cast<GretlBundlePtr>());
        } else {
            throw std::invalid_argument("'" + key_str + "': unsupported type");
        }
    }
}

GretlBundle::~GretlBundle (void)
{
    if (free_b_ && b_ != nullptr) {
        gretl_bundle_destroy(b_);
        b_ = nullptr;
    }
}

template <typename F>
void GretlBundle::for_each_bundle_element_ (F &&f)
{
    //
    // helper fo extract_() methods
    // the 'f' is used to perform desired operation
    //
    if (!gretl_bundle_has_content(b_)) return;
    int n_keys = 0;
    char **raw_ptr = gretl_bundle_get_keys_raw(b_, &n_keys);

    if (!raw_ptr) throw std::runtime_error("failed to obtain bundle keys");
    if (n_keys == 0) return;

    strings_array_ptr keys(raw_ptr, StringsArrayDeleter{});

    keys.get_deleter().nstrs = n_keys;
    for (int i=0; i<n_keys; ++i) {
        const char *key_cstr = keys.get()[i];
        py::str key = py::cast(key_cstr);

        f(key, key_cstr);
    }
}

void GretlBundle::extract_ (const py::object &obj)
{
    //
    // we register bundle contents as attributes of given obj
    //
    for_each_bundle_element_([&](py::str, const char *key_cstr) {
        py::setattr(obj, key_cstr, get_element<py::object>(key_cstr));
    });
}

void GretlBundle::extract_ (py::dict &dict)
{
    //
    // we extrac bundle to given dict
    //
    for_each_bundle_element_([&](py::str key, const char *key_cstr) {
        py::object val = get_element<py::object>(key_cstr);

        if (py::isinstance<GretlBundle>(val)) {
            dict[key] = val.cast<GretlBundle>().to_dict();
        } else {
            dict[key] = std::move(val);
        }
    });
}

int GretlBundle::get_err (void) const
{
    return err_;
}

py::dict GretlBundle::to_dict (void)
{
    extract_(as_dict_);

    return as_dict_;
}

void GretlBundle::python_register (py::object &obj)
{
    extract_(obj);
}

template <typename T>
T GretlBundle::print (bool tree)
{
    //
    // dedicated printer:
    // note: if 'tree', we print bundle tree representation
    //
    GretlPrint printer = GretlPrint();

    // we do the job
    if (tree) {
        err_ = gretl_bundle_print_tree(b_, printer.prn_ptr());
    } else {
        err_ = gretl_bundle_print(b_, printer.prn_ptr());
    }
    if (err_) handle_gretl_error(err_);

    // we either just print or return buffer
    if constexpr (std::is_same_v<T, void>) {
        printer.print();
    } else if constexpr (std::is_same_v<T, std::string>) {
        return printer.get_buffer();
    } else {
        static_assert(always_false<T>, "GretlBundle::print(): unknown type");
    }
}
template void GretlBundle::print<void>(bool);

std::string GretlBundle::repr_impl (void)
{
    if (err_) return "";

    return print<std::string>();
}

gretl_bundle *GretlBundle::get_ptr (void) const
{
    return b_;
}

py::list GretlBundle::keys (void)
{
    //
    // we return bundle keys as a Python list
    //
    // early exit if bundle is empty
    if (!gretl_bundle_has_content(b_)) return py::list{};

    int n_keys = 0;
    char **raw_ptr = gretl_bundle_get_keys_raw(b_, &n_keys);

    if (!raw_ptr) throw std::runtime_error("failed to obtain bundle keys");
    strings_array_ptr keys(raw_ptr, StringsArrayDeleter{});

    keys.get_deleter().nstrs = n_keys;
    if (n_keys == 0) return py::list{};

    return c_style_char_array_to_container<py::list>(keys.get(), n_keys);
}

template <typename T>
T GretlBundle::get_element (const std::string &key)
{
    //
    // we get bundle element by name (key)
    // and return either py::object or GretlTypesPtr
    //
    GretlType gtype = gretl_bundle_get_member_type(b_, key.c_str(), &err_);

    if (err_) handle_gretl_error(err_);
    auto *data = gretl_bundle_get_target(b_, key.c_str(), &gtype, &err_);

    if (err_) handle_gretl_error(err_);
    auto val = convert_GretlType_to_GretlTypes(gtype == GRETL_TYPE_SERIES ? GRETL_TYPE_MATRIX : gtype, data, 0, err_);

    if (err_) handle_gretl_error(err_);
    if constexpr (std::is_same_v<T, py::object>) {
        return variant_to_pyobject(std::move(*val));
    } else if constexpr (std::is_same_v<T, GretlTypesPtr>) {
        return val;
    } else {
        static_assert(always_false<T>, "GretlBundle::get_element: unknown type");
    }
}
template py::object GretlBundle::get_element<py::object>(const std::string&);
template GretlTypesPtr GretlBundle::get_element<GretlTypesPtr>(const std::string&);

template <typename T>
void GretlBundle::set_element (const std::string &key, const T &t)
{
    //
    // we set bundle element by name (key)
    //
    using U = std::decay_t<T>;

    if constexpr (std::is_same_v<U, bool> || std::is_same_v<U, int>) {
        err_ = gretl_bundle_set_int(b_, key.c_str(), t);
    } else if constexpr (std::is_same_v<U, double>) {
        err_ = gretl_bundle_set_scalar(b_, key.c_str(), t);
    } else if constexpr (std::is_same_v<U, std::string>) {
        err_ = gretl_bundle_set_string(b_, key.c_str(), t.c_str());
    } else if constexpr (std::is_same_v<U, GretlBundlePtr>) {
        err_ = gretl_bundle_set_data(b_, key.c_str(), t->get_ptr(), GRETL_TYPE_BUNDLE);
    } else if constexpr (std::is_same_v<U, GretlMatrixPtr>) {
        err_ = gretl_bundle_set_data(b_, key.c_str(), t->get_ptr(), GRETL_TYPE_MATRIX);
    } else if constexpr (std::is_same_v<U, py::array>) {
        NumPyArrayView arr_view;
        gretl_matrix_ptr mat{nullptr};

        NumPyArray_extract(t, arr_view);
        if (!arr_view.data) throw std::runtime_error("NumPy array: null pointer.");
        mat.reset(gretl_matrix_alloc(arr_view.rows, arr_view.cols));

        NumPyArray_to_gretl_storage<gretl_matrix>(arr_view, mat.get());
        err_ = gretl_bundle_donate_data(b_, key.c_str(), mat.release(), GRETL_TYPE_MATRIX);
    } else {
        static_assert(always_false<U>, "GretlBundle::set_element(): unknown type");
    }
    if (err_) handle_gretl_error(err_);
}
template void GretlBundle::set_element<bool>(const std::string&, const bool&);
template void GretlBundle::set_element<int>(const std::string&, const int&);
template void GretlBundle::set_element<double>(const std::string&, const double&);
template void GretlBundle::set_element<std::string>(const std::string&, const std::string&);
template void GretlBundle::set_element<py::array>(const std::string&, const py::array&);
template void GretlBundle::set_element<GretlBundlePtr>(const std::string&, const GretlBundlePtr&);
template void GretlBundle::set_element<GretlMatrixPtr>(const std::string&, const GretlMatrixPtr&);

void GretlBundle::delete_element (const std::string &key)
{
    //
    // deletes the data item under @key if such an item is present
    //
    err_ = gretl_bundle_delete_data(b_, key.c_str());
    if (err_) handle_gretl_error(err_);
}

py::object GretlBundle::pop (const std::string &key)
{
    //
    // method which mimics python's pop(), i.e.
    // retrieves and deletes element under @key
    //
    py::object res = get_element<py::object>(key);
    delete_element(key);

    return res;
}
