/*
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  gretl4py -- Python bindings for gretl
 *  Copyright (C) 2024 Marcin Błażejowski
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

#ifndef GRETL4PY_UTILS_H
#define GRETL4PY_UTILS_H

#include "gretl4py_types.h"
#include "gretl4py_dataset.h"
#include "gretl4py_matrix.h"
#include "gretl4py_bundle.h"
#include <filesystem>

namespace py = pybind11;

//////////////////////////////////////////////////
// macros
//////////////////////////////////////////////////
#define SET_OPT(opt, flag) opt = static_cast<gretlopt>((opt) | (flag))

//////////////////////////////////////////////////
// exceptions
//////////////////////////////////////////////////
struct file_not_found : std::exception {
    //
    // FileNotFoundError
    //
    std::string msg;

    explicit file_not_found (const std::string &f) : msg(f) {}
    const char *what() const noexcept override { return msg.c_str(); }
};

//////////////////////////////////////////////////
// inline functions
//////////////////////////////////////////////////
inline std::complex<double> c_complex_to_std_complex (const double _Complex z) { return {__real__ z, __imag__ z}; }

inline double _Complex std_complex_to_c_complex (std::complex<double> z) { return {z.real(), z.imag()}; }

// Python type checks
inline bool is_pybool (const py::handle &o)    { return py::isinstance<py::bool_>(o); }
inline bool is_pyint (const py::handle &o)     { return py::isinstance<py::int_>(o); }
inline bool is_pyfloat (const py::handle &o)   { return py::isinstance<py::float_>(o); }
inline bool is_pystr (const py::handle &o)     { return py::isinstance<py::str>(o); }
inline bool is_pydict (const py::handle &o)    { return py::isinstance<py::dict>(o); }
inline bool is_pytuple (const py::handle &o)   { return py::isinstance<py::tuple>(o); }
inline bool is_pylist (const py::handle &o)    { return py::isinstance<py::list>(o); }
inline bool is_pynumarray (const py::handle &o){ return py::isinstance<py::array>(o); }
inline bool is_pynumarrayd (const py::handle &o){ return py::isinstance<py::array_t<double>>(o); }
inline bool is_pynumeric (const py::handle &o) { return is_pyint(o) || is_pyfloat(o); }
inline bool is_pysequence (const py::handle &o) { return py::isinstance<py::sequence>(o); }
inline bool is_pyslice (const py::handle &o) { return py::isinstance<py::slice>(o); }
inline bool is_pynone (const py::handle &o) { return py::isinstance<py::none>(o); }
inline bool is_pybytes (const py::handle &o) { return py::isinstance<py::bytes>(o); }
inline bool is_numpy_array_fc (const py::array &m) {
    return (m.flags() & py::detail::npy_api::constants::NPY_ARRAY_F_CONTIGUOUS_) != 0;
}
inline bool is_pygretlbundle (const py::handle &o) { return py::isinstance<GretlBundle>(o); }
inline bool is_pygretlmatrix (const py::handle &o) { return py::isinstance<GretlMatrix>(o); }

//////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////
//
// Class to handle plotting
//
class GretlPlot
{
private:
    static std::filesystem::path executor_;     // path to gnuplot replacement
    static std::filesystem::path queue_dir_;    // queue path (handled automatically)
    static unsigned long long queue_owner_pid_; // which process owns queue
    static bool auto_show_;                     // whether we show plots for executed scripts

    std::filesystem::path gpfile_;              // path to the source gnuplot script
    std::filesystem::path pyfile_;              // path to the generated Python script
    bool temporary_ = false;                    // whether generated files are temporary
    bool source_owned_ = false;                 // whether GretlPlot owns the source file
    bool display_ = true;                       // whether to execute and display the plot

    static unsigned long long current_pid_ (void) noexcept;

    static std::string random_plot_name (void);

    static py::object execute_script_ (const std::filesystem::path &script);

    static void show_figures_ (void);

    static void cleanup_queue_ (void) noexcept;
public:
    GretlPlot (void) = default;
    GretlPlot (const GretlPlot&) = delete;
    ~GretlPlot (void);

    GretlPlot &operator= (const GretlPlot&) = delete;

    static void init_executor (void);

    static void init_queue (void);

    static void drain_queue (void);

    [[nodiscard]] static std::string get_executor_path (void);

    static void set_auto_show (bool enabled) noexcept;

    [[nodiscard]] static bool get_auto_show (void) noexcept;

    std::string filename (void);

    bool translate (void);

    py::object display (void);

    void set_source (const std::filesystem::path &path);

    void store (const std::filesystem::path &path);

    void add_band (TemporaryUserVar &uv_tmp, py::kwargs &kwargs);

    void add_bands (TemporaryUserVar &uv_tmp, py::kwargs &kwargs);
};

//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////
std::string gretl4py_trim (std::string &s) noexcept;

std::string gretl4py_to_lower (std::string s) noexcept;

void NumPyArray_extract (const py::array &arr, NumPyArrayView &view);

py::object variant_to_pyobject (GretlTypes &&val) noexcept;

template <typename T>
void get_object_from_dict (const py::dict &dict, const char *key, T &target);

template <bool take_ownership = false>
GretlTypesPtr convert_GretlType_to_GretlTypes (GretlType t, void *gr_obj_ptr, const int n, int &err);

PythonArgInfo inspect_pyobject (py::handle obj);

template <typename PyWrapped, typename Item, typename ContainerType>
void move_to_python_container (PyWrapped &wrapped_obj, Item &&item, const char *container, const std::string &key = "");

template <typename T>
void load_string_to_container (T &t, const std::string &src);

bool is_bool_kwarg (const py::handle &h, const std::string &key = "");

std::string get_pyobject_type (const py::handle &obj);

template <typename T>
MatrixData<T> python_sequence_to_matrix_data (const py::sequence &seq);

template <typename T>
T c_style_char_array_to_container (c_str_array src, int size);

// adapters to c_style_char_array_to_container()
template <typename T>
T c_style_char_array_to_container (const char **src, int size);

template <typename T>
T c_style_char_array_to_container (char **src, int size);

template <typename T>
void NumPyArray_to_gretl_storage (NumPyArrayView &view, T *dst, int offset = 0, bool onecol = false);

std::string gretl_slice_from_int (int val);

GretlSlice python_slice_to_gretl_slice (const py::slice &slice);

GretlSlice python_slice_to_gretl_slice (const std::string &slice);

// Determine the type of a Python list, validating nested lists recursively.
GretlType get_pyarray_type (const py::list &items);

gretl_array_ptr python_list_to_gretl_array (const py::list &items, GretlType type = GRETL_TYPE_NONE);

#endif // GRETL4PY_UTILS_H
