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
#include <pybind11/eval.h>
#include "gretl4py_utils.h"
#include "gretl4py_types.h"
#include "gretl4py_dataset.h"
#include "gretl4py_model.h"
#include "gretl4py_bundle.h"
extern "C" {
    #include <gretl/gretl_typemap.h>
}
#include <limits>
#include <random>
#include <vector>
#include <cstdlib>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace py = pybind11;

namespace {
// T == source type
template <typename T>
void real_NumPyArray_to_gretl_storage (const NumPyArrayView &view, double *dst, int offset, bool onecol)
{
    //
    // Convert a NumPy array (T) to gretl_matrix (double*).
    // - offset: column offset (used when copying a single column)
    // - onecol: true if we're copying one column only
    //
    const T *src = static_cast<const T*>(view.data);
    const bool fast_path = view.NumPyArrayFC && std::is_same_v<T, double>;

    if (onecol) {
        if (fast_path) {
            std::memcpy(dst, src + offset * view.rows, view.rows * sizeof(double));
        } else {
            for (int i=0; i<view.rows; ++i) {
                dst[i] = static_cast<double>(src[i * view.row_stride + offset * view.col_stride]);
            }
        }
    } else {
        if (fast_path) {
            std::memcpy(dst, src, view.rows * view.cols * sizeof(double));
        } else {
            for (int j=0; j<view.cols; ++j) {
                for (int i=0; i<view.rows; ++i) {
                    dst[j * view.rows + i] = static_cast<double>(src[i * view.row_stride + j * view.col_stride]);
                }
            }
        }
    }
}

void complex_NumPyArray_to_gretl_matrix (const NumPyArrayView &view, gretl_matrix *dst)
{
    //
    // Convert a NumPy std::complex<double> array to gretl_matrix (double _Complex*)
    //
    if (!dst->is_complex) throw std::invalid_argument("matrix must be complex");
    const auto *src = static_cast<const std::complex<double>*>(view.data);

    for (int j=0; j<view.cols; ++j) {
        for (int i=0; i<view.rows; ++i) {
            const auto val = src[i * view.row_stride + j * view.col_stride];
            double _Complex z = { val.real(), val.imag() };

            gretl_cmatrix_set(dst, i, j, z);
        }
    }
}

inline GretlSlice make_gretl_slice (std::string startobs, std::string endobs, std::string slice_str, py::dict opts = {})
{
    //
    // small helper for fast GretlSlice generation
    //
    GretlSlice gs;

    gs.startobs = std::move(startobs);
    gs.endobs = std::move(endobs);
    gs.slice_str = std::move(slice_str);
    gs.opts = std::move(opts);

    return gs;
}
}

//////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////
//
// GretlPlot
//
// Static path to the gp-executor binary.
std::filesystem::path GretlPlot::executor_;
std::filesystem::path GretlPlot::queue_dir_;
unsigned long long GretlPlot::queue_owner_pid_ = 0;
bool GretlPlot::auto_show_ = true;

//
// GretlPlot class methods
//
GretlPlot::~GretlPlot (void)
{
    // Remove only temporary files owned by GretlPlot.
    if (temporary_) {
        std::error_code ec;

        if (source_owned_ && !gpfile_.empty()) {
            std::filesystem::remove(gpfile_, ec);
        }

        if (!pyfile_.empty()) {
            std::filesystem::remove(pyfile_, ec);
        }
    }
}

//
// static methods
//
void GretlPlot::cleanup_queue_ (void) noexcept
{
    //
    // Remove only the plot queue owned by this process.
    // Never remove the shared parent directory or other sessions.
    // Inherited atexit handlers must not delete the parent's queue.
    //
    if (queue_dir_.empty() || queue_owner_pid_ != current_pid_()) return;

    std::error_code ec;
    std::filesystem::remove_all(queue_dir_, ec);

    // Cleanup must not throw during process termination.
    queue_dir_.clear();
    queue_owner_pid_ = 0;
}

std::string GretlPlot::random_plot_name (void)
{
    //
    // generate a random filename with a 128-bit identifier to minimize
    // collisions between independent processes, including MPI workers
    //
    // generate a random 128-bit identifier
    std::random_device rd;
    std::array<unsigned int, 4> words;

    for (auto &word : words) {
        word = rd();
    }

    std::ostringstream os;
    os << "gptranslate_";

    for (const auto word : words) {
        os << std::hex << std::setw(8)
        << std::setfill('0') << word;
    }

    os << ".gp";

    return os.str();
}

unsigned long long GretlPlot::current_pid_ (void) noexcept
{
    //
    // Identify the current process, including a child created by fork().
    // return the current OS process identifier on Windows and POSIX systems
    //
    #ifdef _WIN32
    return static_cast<unsigned long long>(GetCurrentProcessId());
    #else
    return static_cast<unsigned long long>(getpid());
    #endif
}

py::object GretlPlot::execute_script_(const std::filesystem::path &script)
{
    //
    // Execute a generated matplotlib script; the caller must hold the GIL
    // interpreter, using a dedicated namespace.
    //
    py::dict globals;

    globals["__name__"] = "__main__";
    globals["__file__"] = script.string();

    py::eval_file(script.string(), globals);

    // Retrieve the Figure explicitly created by gptranslate.
    // Keep a Python reference so the object remains accessible
    // after the script has finished executing.
    if (globals.contains("fig")) return globals["fig"].cast<py::object>();

    return py::none();
}

void GretlPlot::show_figures_ (void)
{
    //
    // Display pending figures and wait for their windows to close; caller holds the GIL
    //
    py::module_ plt = py::module_::import("matplotlib.pyplot");

    plt.attr("show")(py::arg("block") = false);
}

void GretlPlot::set_auto_show (bool enabled) noexcept
{
    // Set the global automatic plot display policy.
    auto_show_ = enabled;
}

bool GretlPlot::get_auto_show (void) noexcept
{
    return auto_show_;
}

//
// instance methods
//
void GretlPlot::init_executor (void)
{
    // Locate gp-executor in the gretl4py installation directory.
    executor_ = do_get_accessor<GRETL_OBJ_ANY>("$gretldir", GretlDataset::default_dataset(), nullptr).cast<std::string>();

#ifdef _WIN32
    executor_ /= "gp-executor.exe";
#else
    executor_ /= "gp-executor";
#endif

    if (!std::filesystem::is_regular_file(executor_)) throw std::runtime_error("gp-executor not found: " + executor_.string());
}

void GretlPlot::init_queue (void)
{
    //
    // Initialize one private queue per Python process. The random directory
    // name prevents collisions between independent processes and MPI workers.
    //
    const auto pid = current_pid_();

    if (!queue_dir_.empty() && queue_owner_pid_ == pid) return;

    // A forked child inherits the parent's static path, but must create
    // its own queue rather than reuse or delete the parent's directory.
    if (queue_owner_pid_ != pid) {
        queue_dir_.clear();
        queue_owner_pid_ = 0;
    }

    const auto root = std::filesystem::path(gretl_dotdir()) / "queue";
    std::filesystem::create_directories(root);

    std::random_device rd;
    std::filesystem::path candidate;
    bool created = false;

    for (int attempt = 0; attempt < 32; ++attempt) {
        std::ostringstream name;
        name << "session_" << std::hex << rd() << rd() << rd() << rd();
        candidate = root / name.str();
        if (std::filesystem::create_directory(candidate)) {
            created = true;
            break;
        }
    }
    if (!created) throw std::runtime_error("cannot create a unique plot queue directory");

    // gp-executor is a separate process and receives this path via its environment.
    // Preserve the previous value so a failed initialization can restore it.
    const char *previous = std::getenv("GRETL4PY_PLOT_QUEUE");
    const bool had_previous = previous != nullptr;
    const std::string previous_value = had_previous ? previous : "";
    const std::string path = candidate.string();
#ifdef _WIN32
    if (_putenv_s("GRETL4PY_PLOT_QUEUE", path.c_str()) != 0) {
#else
    if (setenv("GRETL4PY_PLOT_QUEUE", path.c_str(), 1) != 0) {
#endif
        std::error_code ec;
        std::filesystem::remove(candidate, ec);
        throw std::runtime_error("cannot set GRETL4PY_PLOT_QUEUE");
    }
    queue_dir_ = std::move(candidate);
    queue_owner_pid_ = pid;

    // Register cleanup after successful queue initialization. A forked child
    // may register another handler; the PID check makes inherited handlers safe.
    // The static queue_dir_ is initialized before this registration, so its
    // destructor runs after this cleanup handler during normal termination.
    if (std::atexit(&GretlPlot::cleanup_queue_) != 0) {
        cleanup_queue_();

        // Restore the inherited or previous environment on registration failure.
#ifdef _WIN32
        if (had_previous) {
            _putenv_s("GRETL4PY_PLOT_QUEUE", previous_value.c_str());
        } else {
            _putenv_s("GRETL4PY_PLOT_QUEUE", "");
        }
#else
        if (had_previous) {
            setenv("GRETL4PY_PLOT_QUEUE", previous_value.c_str(), 1);
        } else {
            unsetenv("GRETL4PY_PLOT_QUEUE");
        }
#endif
        throw std::runtime_error("cannot register plot queue cleanup");
    }
}

void GretlPlot::drain_queue (void)
{
    //
    // only complete .py files are published by gp-executor; .tmp files
    // are deliberately ignored until their atomic rename has completed
    // A forked child must not drain scripts belonging to its parent.
    //
    if (queue_dir_.empty() || queue_owner_pid_ != current_pid_()) return;

    std::vector<std::filesystem::path> scripts;
    for (const auto &entry : std::filesystem::directory_iterator(queue_dir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".py")
            scripts.push_back(entry.path());
    }
    if (scripts.empty()) return;

    // gp-executor prefixes filenames with a padded creation timestamp.
    std::sort(scripts.begin(), scripts.end());

    py::gil_scoped_acquire gil;
    for (const auto &script : scripts) {
        // keep a failed script for inspection; remove only successful ones
        execute_script_(script);
        std::filesystem::remove(script);
    }

    // display all figures after evaluating every script in this batch
    show_figures_();
}

[[nodiscard]] std::string GretlPlot::get_executor_path (void)
{
    return executor_.string();
}

std::string GretlPlot::filename (void)
{
    //
    // generate and store a unique temporary gnuplot filename
    //
    gpfile_ = std::filesystem::path(gretl_dotdir()) / random_plot_name();

    pyfile_ = gpfile_;
    pyfile_.replace_extension(".py");

    temporary_ = true;
    source_owned_ = true;

    return gpfile_.string();
}

bool GretlPlot::translate (void)
{
    //
    // translate a generated gnuplot script into a matplotlib Python script
    // return false if libgretl did not generate a plot
    //
    if (gpfile_.empty() || !std::filesystem::is_regular_file(gpfile_)) return false;

    // translate the gnuplot script without executing Python (execute command)
    const auto &executor = executor_;
    std::string command = "\"" + executor.string() + "\"";

#ifdef _WIN32
    // Select PySide6/QtAgg in generated Python scripts on Windows.
    command += " --backend=QtAgg";
#endif

    // gretl4py manages plot display; suppress plt.show() in generated scripts.
    // Keep plt.show() in standalone Python scripts saved to disk.
    if (display_) command += " --no-show";

    command += " --output=\"" + pyfile_.string() + "\" \"" + gpfile_.string() + "\"";

#ifdef _WIN32
    // cmd.exe requires an additional pair of quotes around the entire command
    command = "\"" + command + "\"";
#endif

    const int status = std::system(command.c_str());

    if (status != 0 || !std::filesystem::is_regular_file(pyfile_)) throw std::runtime_error("gptranslate failed: " + gpfile_.string());

    return true;
}

py::object GretlPlot::display()
{
    // Translate the plot before displaying it.
    if (!translate()) return py::none();

    // Do not display plots explicitly saved to disk.
    if (!display_) return py::none();

    // Execute the generated matplotlib script in the current Python interpreter.
    py::gil_scoped_acquire gil;

    // Preserve LC_NUMERIC across Matplotlib backend initialization.
    struct NumericLocaleGuard {
        std::string saved_;

        NumericLocaleGuard()
        {
            const char *loc = std::setlocale(LC_NUMERIC, nullptr);

            if (loc) saved_ = loc;
        }

        ~NumericLocaleGuard()
        {
            if (!saved_.empty()) std::setlocale(LC_NUMERIC, saved_.c_str());
        }
    } locale_guard;

    py::object fig = execute_script_(pyfile_);

    // Display all figures using the selected Matplotlib backend.
    if (auto_show_) show_figures_();

    return fig;
}

void GretlPlot::set_source (const std::filesystem::path &path)
{
    // Validate the user-supplied gnuplot script.
    if (!std::filesystem::is_regular_file(path)) throw file_not_found("Plot file not found: " + path.string());

    // Preserve the original file and generate Python in the temporary directory.
    gpfile_ = std::filesystem::absolute(path);

    pyfile_ = std::filesystem::path(gretl_dotdir()) / random_plot_name();
    pyfile_.replace_extension(".py");

    temporary_ = true;
    source_owned_ = false;
}

void GretlPlot::store (const std::filesystem::path &path)
{
    //
    // preserve the user-specified plot and its Python translation
    //
    gpfile_ = std::filesystem::absolute(path);
    pyfile_ = gpfile_;
    pyfile_.replace_extension(".py");

    temporary_ = false;
    source_owned_ = false;
    display_ = false;
}

//////////////////////////////////////////////////
// free functions
//////////////////////////////////////////////////
std::string gretl4py_trim (std::string &s) noexcept
{
    //
    // general purpose trimer
    //
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());

    return s;
}

std::string gretl4py_to_lower (std::string s) noexcept
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
    if (arr.ndim() == 0 || arr.ndim() > 2) throw std::runtime_error("Given array must have one or two dimensions.");

    // arr.shape() is a 64-bit py::ssize_t; NumPyArrayView::rows/cols are int
    // (fed directly into gretl_matrix_alloc()'s int rows/cols and into
    // memcpy-sized copies) - reject anything that would silently narrow/wrap
    // instead of trusting it.
    // Note: spelled Py_ssize_t, not py::ssize_t/ssize_t - on MSYS2/clang,
    // <unistd.h> #defines ssize_t as a macro (__int64), and the preprocessor
    // rewrites that token wherever it appears, including inside py::ssize_t,
    // which then fails to parse as a type.
    auto shape_to_int = [](Py_ssize_t n) -> int
    {
        if (n < 0 || n > (std::numeric_limits<int>::max)()) throw std::overflow_error("NumPy array dimension too large.");

        return static_cast<int>(n);
    };

    view.is_double = arr.dtype().is(py::dtype::of<double>());
    view.is_complex = arr.dtype().is(py::dtype::of<std::complex<double>>());
    view.dtype = arr.dtype();
    view.base_array = arr;          // keep the NumPy array alive
    view.data = arr.request().ptr;
    view.rows = shape_to_int(arr.shape(0));
    view.cols = arr.ndim() == 1 ? 1 : shape_to_int(arr.shape(1));
    view.NumPyArrayFC = is_numpy_array_fc(arr);

    const Py_ssize_t itemsize = view.dtype.itemsize();
    const auto &strides = arr.strides();

    view.row_stride = strides[0] / itemsize;
    view.col_stride = (arr.ndim() == 1) ? 0 : strides[1] / itemsize;
}

// Note: this is needed to avoid memory leaks when
// transferring GretlTypes to Python inside py::dict
py::object variant_to_pyobject (GretlTypes &&val) noexcept
{
    return std::visit([](auto &&arg) -> py::object
    {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_base_of_v<py::object, T>) {
            return std::move(arg);              // steal py::dict, py::list, etc.
        } else {
            return py::cast(std::move(arg));    // move std::string, etc.
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
    auto key_not_a = [key](const char *obj_type) -> void
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

template <bool take_ownership>
GretlTypesPtr convert_GretlType_to_GretlTypes (GretlType t, void *gr_obj_ptr, const int n, int &err)
{
    //
    // Convert a libgretl object to a GretlTypesPtr.
    //
    // Parameters:
    //   t              - Type of the source libgretl object.
    //   gr_obj_ptr     - Pointer to the source object.
    //   n              - Number of observations (relevant for series).
    //   err            - Error code, set if conversion fails.
    //   take_ownership - Whether to take ownership of the source object
    //                    (default: false).
    //
    // Ownership rules:
    //
    // - GRETL_TYPE_NONE and GRETL_TYPE_VOID may have a null source pointer.
    //   For all other types, a null pointer raises E_NODATA.
    //
    // - If take_ownership is false, the source object is borrowed and
    //   remains owned by the caller. The converter must not modify or
    //   destroy it.
    //
    // - If take_ownership is true, the converter assumes responsibility
    //   for the source object, including when conversion fails.
    //
    // - For GRETL_TYPE_MATRIX and GRETL_TYPE_BUNDLE, ownership can be
    //   transferred directly to the corresponding C++ wrapper without
    //   copying the underlying object.
    //
    // - For GRETL_TYPE_STRING, GRETL_TYPE_LIST, GRETL_TYPE_SERIES and
    //   GRETL_TYPE_ARRAY, the converter creates an independent result
    //   and releases the source object using the appropriate deleter.
    //   Array elements remain owned by their parent array during
    //   recursive conversion.
    //
    // - GRETL_TYPE_INT and GRETL_TYPE_DOUBLE are copied by value.
    //   If take_ownership is true, the source allocation is freed
    //   after conversion.
    //
    // The returned GretlTypesPtr owns its converted data independently
    // of the source object.
    //
    // Ownership transfer must be exception-safe. A source object taken
    // into ownership must be released exactly once, even if conversion
    // fails. Calling release() is permitted only when ownership is
    // actually transferred to another object.
    //
    auto res = std::make_unique<GretlTypes>();

    // we early reject bad results
    if (!gr_obj_ptr && t != GRETL_TYPE_NONE && t != GRETL_TYPE_VOID) {
        err = E_NODATA;
        handle_gretl_error(err);
    }

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
        auto *raw = static_cast<int*>(gr_obj_ptr);
        gretl_int_ptr ptr(take_ownership ? raw : nullptr);

        *res = *raw;
        break;
    }
    case GRETL_TYPE_DOUBLE: {
        auto *raw = static_cast<double*>(gr_obj_ptr);
        gretl_double_ptr ptr(take_ownership ? raw : nullptr);

        *res = *raw;
        break;
    }
    case GRETL_TYPE_STRING: {
        auto *raw = static_cast<char*>(gr_obj_ptr);

        if constexpr (take_ownership) {
            gretl_char_ptr ptr(raw);

            *res = std::string(ptr.get());
        } else {
            *res = std::string(raw);
        }
        break;
    }
    case GRETL_TYPE_MATRIX: {
        auto *raw = static_cast<gretl_matrix*>(gr_obj_ptr);

        if constexpr (take_ownership) {
            gretl_matrix_ptr ptr(raw);
            auto obj = std::make_shared<GretlMatrix>(ptr.get(), false);

            ptr.release();
            *res = std::move(obj);
        } else {
            *res = std::make_shared<GretlMatrix>(raw, true);
        }
        break;
    }
    case GRETL_TYPE_BUNDLE: {
        auto *raw = static_cast<gretl_bundle*>(gr_obj_ptr);

        if constexpr (take_ownership) {
            gretl_bundle_ptr ptr(raw);
            auto obj = std::make_shared<GretlBundle>(ptr.get(), false);

            ptr.release();
            *res = std::move(obj);
        } else {
            *res = std::make_shared<GretlBundle>(raw, true);
        }
        break;
    }
    case GRETL_TYPE_ARRAY:
    case GRETL_TYPE_MATRICES:
    case GRETL_TYPE_STRINGS:
    case GRETL_TYPE_BUNDLES: {
        auto *a = static_cast<gretl_array*>(gr_obj_ptr);
        gretl_array_ptr ptr(take_ownership ? a : nullptr);
        py::list py_list;
        GretlType tmp;

        for (int i=0; i<gretl_array_get_length(a); ++i) {
            tmp = gretl_array_get_content_type(a);
            auto element = convert_GretlType_to_GretlTypes<false>(tmp, gretl_array_get_element(a, i, &tmp, &err), n, err);

            if (err) handle_gretl_error(err);

            py_list.append(variant_to_pyobject(std::move(*element)));
        }
        *res = py_list;
        break;
    }
    case GRETL_TYPE_SERIES: {
        auto *raw = static_cast<double*>(gr_obj_ptr);
        gretl_double_ptr ptr(take_ownership ? raw : nullptr);

        *res = std::make_shared<GretlMatrix>(raw, n);
        break;
    }
    case GRETL_TYPE_LIST: {
        auto *raw = static_cast<int*>(gr_obj_ptr);
        gretl_list_ptr ptr(take_ownership ? raw : nullptr);
        py::list py_list;

        for (int i = 1; i <= raw[0]; ++i) {
            py_list.append(raw[i]);
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
template GretlTypesPtr convert_GretlType_to_GretlTypes<false>(GretlType, void*, const int, int&);
template GretlTypesPtr convert_GretlType_to_GretlTypes<true>(GretlType, void*, const int, int&);

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
MatrixData<T> python_sequence_to_matrix_data (const py::sequence &seq)
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
template MatrixData<std::complex<double>> python_sequence_to_matrix_data<std::complex<double>>(const py::sequence&);
template MatrixData<double> python_sequence_to_matrix_data<double>(const py::sequence&);

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

template <typename T>
void NumPyArray_to_gretl_storage (NumPyArrayView &view, T *dst, int offset, bool onecol)
{
    //
    // here we copy contents of some NumPy array described by &view
    // to storage available at *dst
    // offset here is used when we copy to DATASET
    //
    double *storage = nullptr;

    if constexpr (std::is_same_v<T, gretl_matrix>) {
        storage = dst->val;
    } else {
        storage = dst;
    }

    if (view.is_double) {
        real_NumPyArray_to_gretl_storage<double>(view, storage, offset, onecol);
    } else if (view.dtype.is(py::dtype::of<float>())) {
        real_NumPyArray_to_gretl_storage<float>(view, storage, offset, onecol);
    } else if (view.dtype.is(py::dtype::of<int>())) {
        real_NumPyArray_to_gretl_storage<int>(view, storage, offset, onecol);
    } else if (view.dtype.is(py::dtype::of<int64_t>())) {
        real_NumPyArray_to_gretl_storage<int64_t>(view, storage, offset, onecol);
    } else if (view.is_complex) {
        if constexpr (std::is_same_v<T, gretl_matrix>) {
            complex_NumPyArray_to_gretl_matrix(view, reinterpret_cast<gretl_matrix*>(dst));
        } else {
            throw std::runtime_error("Complex NumPy -> non-complex destination not supported");
        }
    } else {
        throw std::runtime_error("Unsupported NumPy dtype for conversion.");
    }
}
template void NumPyArray_to_gretl_storage<double>(NumPyArrayView&, double*, int, bool);
template void NumPyArray_to_gretl_storage<gretl_matrix>(NumPyArrayView&, gretl_matrix*, int, bool);

std::string gretl_slice_from_int (int val)
{
    //
    // here we manufacture gretl slicing string based on given int value (including negative ones)
    //
    std::string slice_str;

    if (val == -1) {
        slice_str = "end";
    } else if (val < -1) {
        slice_str = "end" + std::to_string(val + 1); // this is to get 0-based indexing
    } else {
        slice_str = std::to_string(val + 1);
    }

    return slice_str;
}

GretlSlice python_slice_to_gretl_slice (const py::slice &slice)
{
    //
    // we convert Python's slice string accepted by libgretl
    //
    py::object start = slice.attr("start");
    py::object stop = slice.attr("stop");
    py::object step  = slice.attr("step");

    if (!step.is_none()) {
        if (step.cast<float>() != 1.0) throw std::invalid_argument("slice steps other than 1 are not supported");
    }
    std::string startobs = is_pynone(start) ? "1" : gretl_slice_from_int(start.cast<int>());
    std::string endobs = is_pynone(stop) ? "end" : gretl_slice_from_int(stop.cast<int>() - 1);
    std::string slice_str = startobs + ":" + endobs;

    return make_gretl_slice(startobs, endobs, slice_str);
}

GretlSlice python_slice_to_gretl_slice (const std::string &slice)
{
    //
    // we manufacture string accepted by libgretl
    //
    std::string startobs, endobs;
    py::dict opts;
    auto pos = slice.find(':');

    if (pos != std::string::npos && slice.find(':', pos + 1) == std::string::npos) {
        startobs = slice.substr(0, pos);
        endobs   = slice.substr(pos + 1);
    } else {
        startobs = slice;
        endobs   = "";
        opts["restrict"] = true;
    }

    return make_gretl_slice(startobs, endobs, slice, opts);
}

GretlType get_pyarray_type (const py::list &items)
{
    //
    // Determine the gretl array type of a Python list.
    // Nested lists are validated recursively.
    // Empty lists have type GRETL_TYPE_ANY.
    //
    GretlType type = GRETL_TYPE_NONE;

    for (const auto &item : items) {
        GretlType current = GRETL_TYPE_NONE;

        if (is_pygretlbundle(item)) {
            current = GRETL_TYPE_BUNDLES;
        } else if (is_pygretlmatrix(item) || is_pynumarray(item)) {
            current = GRETL_TYPE_MATRICES;
        } else if (is_pystr(item)) {
            current = GRETL_TYPE_STRINGS;
        } else if (is_pylist(item)) {
            // Validate the nested list independently.
            get_pyarray_type(py::reinterpret_borrow<py::list>(item));
            current = GRETL_TYPE_ARRAYS;
        } else {
            throw py::type_error("Unsupported element type in Python list");
        }

        if (type == GRETL_TYPE_NONE) {
            type = current;
        } else if (type != current) {
            throw py::type_error("Python list elements must have a common gretl array type");
        }
    }

    return type == GRETL_TYPE_NONE ? GRETL_TYPE_ANY : type;
}

gretl_array_ptr python_list_to_gretl_array (const py::list &items, GretlType type)
{
    //
    // Convert a Python list into an independently owned gretl array.
    // Nested lists are converted recursively and ownership is managed by RAII.
    //
    // Validate the complete structure before allocating storage.
    const GretlType inferred = get_pyarray_type(items);

    if (type == GRETL_TYPE_NONE || type == GRETL_TYPE_ANY) {
        type = inferred;
    } else if (inferred != GRETL_TYPE_ANY && type != inferred) {
        throw py::type_error("Explicit gretl array type does not match Python list");
    }

    const auto n = py::len(items);

    if (n > (std::numeric_limits<int>::max)()) throw py::value_error("Python list is too large for gretl array");
    int err = 0;
    gretl_array_ptr array(gretl_array_new(type, static_cast<int>(n), &err));

    if (err || !array) handle_gretl_error(err ? err : E_ALLOC);

    int i = 0;

    for (const auto &item : items) {
        err = 0;

        if (is_pygretlbundle(item)) {
            auto bundle = item.cast<GretlBundlePtr>();

            // Copy the Python-owned bundle.
            err = gretl_array_set_element(array.get(), i, bundle->get_ptr(), GRETL_TYPE_BUNDLE, 1);
        } else if (is_pygretlmatrix(item)) {
            auto matrix = item.cast<GretlMatrixPtr>();

            // Copy the Python-owned matrix.
            err = gretl_array_set_element(array.get(), i, matrix->get_ptr(),GRETL_TYPE_MATRIX, 1);
        } else if (is_pynumarray(item)) {
            NumPyArrayView view;

            NumPyArray_extract(py::reinterpret_borrow<py::array>(item), view);

            // Allocate an independent gretl matrix.
            gretl_matrix_ptr matrix(gretl_matrix_alloc(view.rows, view.cols));

            if (!matrix) handle_gretl_error(E_ALLOC);
            NumPyArray_to_gretl_storage<gretl_matrix>(view, matrix.get());

            // Donate the newly allocated matrix to the array.
            err = gretl_array_set_element(array.get(), i, matrix.get(), GRETL_TYPE_MATRIX, 0);
            if (!err) matrix.release();
        } else if (is_pystr(item)) {
            std::string value = py::cast<std::string>(item);

            // libgretl duplicates the string.
            err = gretl_array_set_element(array.get(), i, value.data(), GRETL_TYPE_STRING, 1);
        } else if (is_pylist(item)) {
            // Recursively convert the nested list.
            gretl_array_ptr nested = python_list_to_gretl_array(py::reinterpret_borrow<py::list>(item));

            // Donate the nested array to its parent.
            err = gretl_array_set_element(array.get(), i, nested.get(), GRETL_TYPE_ARRAY, 0);
            if (!err) nested.release();
        }

        if (err) handle_gretl_error(err);
        ++i;
    }

    return array;
}
