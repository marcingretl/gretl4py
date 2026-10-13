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
#include "gretl4py_matrix.h"
#include "gretl4py_types.h"
#include "gretl4py_utils.h"
#include <gretl/matrix_extra.h>
#include <gretl/gretl_drivers.h>
extern "C" {
    #include <gretl/uservar.h>
    // this is because we cannot include gretl_cmatrix.h
    int gretl_cmatrix_print (const gretl_matrix *A, const char *name, PRN *prn);
    gretl_matrix *gretl_cmatrix_multiply (const gretl_matrix *A, const gretl_matrix *B, int *err);
    gretl_matrix *gretl_cmatrix_divide (const gretl_matrix *A, const gretl_matrix *B, GretlMatrixMod mod, int *err);
    gretl_matrix *gretl_cmatrix_add_sub (const gretl_matrix *A, const gretl_matrix *B, int sgn, int *err);
    gretl_matrix *gretl_cmatrix_dot_op (const gretl_matrix *a, const gretl_matrix *b, int op, int *err);
}

//
// GretlMatrix class methods
//
GretlMatrix::GretlMatrix (void) : m_(gretl_null_matrix_new())
{
    //
    // default constructor
    //
    if (!m_) handle_gretl_error(E_ALLOC);
}

GretlMatrix::GretlMatrix (const GretlMatrix &src): m_(gretl_matrix_copy(src.get_ptr()))
{
    //
    // copy constructor
    //
    if (!m_) handle_gretl_error(E_ALLOC);
}

GretlMatrix::GretlMatrix (int rows, int cols, double fill, const std::string &type)
{
    //
    // we allocate matrix which may be filled with @fill
    //
    enum GretlMatrixType { Real, Complex };
    GretlMatrixType dtype;
    const auto ltype = gretl4py_to_lower(type);

    // basic checks
    if (ltype == "real") {
        dtype = Real;
    } else if (ltype == "complex") {
        dtype = Complex;
    } else {
        handle_gretl_error(E_INVARG, "type", "must be 'real' or 'complex'");
    }

    // we allocate real/complex matrix
    if (dtype == Real) {
        m_ = (fill == 0.0) ? gretl_zero_matrix_new(rows, cols) : gretl_unit_matrix_new(rows, cols);
    } else {
        m_ = (fill == 0.0) ? gretl_cmatrix_new0(rows, cols) : gretl_cmatrix_new1(rows, cols);
    }
    if (!m_) handle_gretl_error(E_ALLOC);
    if (fill && fill != 1.0) gretl_matrix_fill(m_, fill);
}

GretlMatrix::GretlMatrix (int rows, int cols, const std::string &dist)
{
    //
    // creates a new matrix and fills it with pseudo-random
    // values from either the uniform or the standard normal distribution
    //
    auto dist_code = dist_code_from_string(dist.substr(0, 1).c_str());

    if (dist_code != D_UNIFORM && dist_code != D_NORMAL) throw std::invalid_argument("unknown distribution code: '" + dist + "'");
    m_ = gretl_random_matrix_new(rows, cols, dist_code);
    if (!m_) handle_gretl_error(E_ALLOC);
}

GretlMatrix::GretlMatrix (double start, double end, double step)
{
    //
    // row vector, containing values from @start to @end, in decreasing order if @start > @end
    //
    m_ = gretl_matrix_seq(start, end, step, &err_);
    if (err_) handle_gretl_error(err_);
}

GretlMatrix::GretlMatrix (int n) : m_(gretl_identity_matrix_new(n))
{
    //
    // identity matrix
    //
    if (!m_) handle_gretl_error(E_ALLOC);
}

GretlMatrix::GretlMatrix (const py::array &array)
{
    //
    // from NumPy source
    //
    m_ = numpy_to_matrix_(array);
}

GretlMatrix::GretlMatrix (gretl_matrix *src, bool copy)
{
    //
    // copy == true means deep copy, false == we "steal" pointer
    //
    copy ? m_ = gretl_matrix_copy(src) : m_ = src;
    if (!m_) handle_gretl_error(E_ALLOC);
}

GretlMatrix::GretlMatrix (const py::sequence &seq)
{
    //
    // we construct GretlMatrix for given sequence
    // Note: memory is allocated inside from_matrix_()
    //
    try {
        from_matrix_(python_sequence_to_matrix_data<double>(seq));
    } catch (const py::cast_error&) {
        from_matrix_(python_sequence_to_matrix_data<std::complex<double>>(seq));
    }
}

GretlMatrix::GretlMatrix (const double *src, int n, GretlMatrixMod mod) : GretlMatrix(gretl_vector_from_array(src, n, mod), false)
{}

GretlMatrix::GretlMatrix (const double **src, int rows, int cols) : GretlMatrix(gretl_matrix_from_2d_array(src, rows, cols), false)
{}

GretlMatrix::~GretlMatrix (void)
{
    if (m_) {
        gretl_matrix_free(m_);
        m_ = nullptr;
    }
}

GretlMatrix &GretlMatrix::operator= (const GretlMatrix &src)
{
    //
    // deep copy: mirrors the copy constructor - printer_/err_ are
    //
    if (this != &src) {
        gretl_matrix *new_m = gretl_matrix_copy(src.get_ptr());

        if (!new_m) handle_gretl_error(E_ALLOC);
        if (m_) gretl_matrix_free(m_);
        m_ = new_m;
    }

    return *this;
}

GretlMatrixPtr GretlMatrix::identity (int n, std::optional<int> m)
{
    //
    // method which mimics I(n,m) gretl function
    //
    //
    if (!m) return std::make_shared<GretlMatrix>(n);
    int cols = *m;

    if (cols < 0) {
        handle_gretl_error(E_INVARG);
    } else if (cols == 0) {
        return std::make_shared<GretlMatrix>();
    } else {
        gretl_matrix_ptr mat{nullptr};
        int err = 0;

        mat.reset(gretl_zero_matrix_new(n, cols));
        if (!mat) handle_gretl_error(E_ALLOC);
        err = gretl_matrix_inscribe_I(mat.get(), 0, 0, n);
        if (err) handle_gretl_error(err);
        return std::make_shared<GretlMatrix>(mat.release(), false);
    }
}

GretlMatrixPtr GretlMatrix::diag (const py::sequence &seq)
{
    //
    // method which mimics NumPy's diag(n) function
    //
    auto diag = GretlMatrix(seq);
    int size = (std::max)(diag.rows(), diag.cols());
    auto res = std::make_shared<GretlMatrix>(size, size, 0.0, "real");

    res->set_diagonal(diag.get_ptr());

    return res;
}

GretlMatrixPtr GretlMatrix::zeros (int rows, int cols)
{
    return std::make_shared<GretlMatrix>(rows, cols, 0.0, "real");
}

//
// private methods
//
// static
gretl_matrix *GretlMatrix::numpy_to_matrix_ (const py::array &array)
{
    //
    // from NumPy source
    // note: the allocated memory will be owned by caller
    //
    NumPyArrayView arr_view;
    gretl_matrix *res = nullptr;

    NumPyArray_extract(array, arr_view);
    if (!arr_view.data) throw std::runtime_error("NumPy array: null pointer.");
    if (arr_view.is_complex) {
        res = gretl_cmatrix_new(arr_view.rows, arr_view.cols);
    } else {
        res = gretl_matrix_alloc(arr_view.rows, arr_view.cols);
    }
    if (!res) handle_gretl_error(E_ALLOC);
    NumPyArray_to_gretl_storage<gretl_matrix>(arr_view, res);

    return res;
}

// instance
void GretlMatrix::check_bounds_ (int i, int j) const
{
    // NOTE std::abs(i) >= rows: std::abs(INT_MIN) is undefined behaviour
    // (negating INT_MIN overflows int - two's complement has one more
    // negative value than positive, so there's no representable +2^31 to
    // return). In practice it just wraps back to INT_MIN, which is still
    // negative, so "abs(i) >= rows" would wrongly evaluate false and let
    // i == INT_MIN slip past this check. Expressing the same "magnitude
    // within bounds" test as two plain comparisons avoids the negation
    // entirely, so it's well-defined for every int, INT_MIN included.
    if (i >= m_->rows || i < -m_->rows || j >= m_->cols || j < -m_->cols) {
        handle_gretl_error(E_BOUNDS, "[i,j]", "[" + std::to_string(i) + "," + std::to_string(j) + "]");
    }
}

void GretlMatrix::check_bounds_ (int i) const
{
    // see check_bounds_(i,j) above for why this avoids std::abs()
    int len = (std::max)(m_->rows, m_->cols);

    if (i >= len || i < -len) handle_gretl_error(E_BOUNDS, "[i]", "[" + std::to_string(i) + "]");
}

gretl_matrix_ptr GretlMatrix::exec_slice_ (const std::string &slice_str) const
{
    //
    // here we execute libgretl apparatus to get sliced matrix
    //
    const std::string tmp = "m_gretlpy__";
    std::string expr = tmp + "[" + slice_str + "]";
    gretl_matrix_ptr res{nullptr};
    int err = 0;

    err_ = private_matrix_add_as_shell(m_, tmp.c_str());
    if (err_) handle_gretl_error(err_);
    res.reset(generate_matrix(expr.c_str(), nullptr, &err));
    err_ = user_var_delete_by_name(tmp.c_str(), nullptr);
    if (err) handle_gretl_error(err);
    if (err_) handle_gretl_error(err_);

    return res;
}

std::string GretlMatrix::render_slice_ (const py::tuple &slice) const
{
    std::string res;

    for (auto item : slice) {
        std::string tmp;

        if (is_pyslice(item)) {
            tmp = python_slice_to_gretl_slice(item.cast<py::slice>()).slice_str;
        } else if (is_pyint(item)) {
            tmp = gretl_slice_from_int(item.cast<int>());
        } else {
            throw std::invalid_argument("bad slicer passed");
        }
        if (!res.empty()) res += ",";
        res += (tmp == ":" ? " " : tmp);
    }

    return gretl4py_trim(res);
}

std::string GretlMatrix::render_slice_ (const py::slice &slice) const
{
    return python_slice_to_gretl_slice(slice).slice_str;
}

std::string GretlMatrix::render_slice_ (const std::string &slice) const
{
    return python_slice_to_gretl_slice(slice).slice_str;
}

// T = {double ; std::complex<double>}
template <typename T>
void GretlMatrix::from_matrix_ (const MatrixData<T> &data)
{
    //
    // helper for filling up gretl_matrix storage from given STL container
    //
    const int rows = static_cast<int>(data.size());
    const int cols = rows ? static_cast<int>(data[0].size()) : 0;

    // we check the shape
    for (const auto &row : data) {
        if (static_cast<int>(row.size()) != cols) throw std::invalid_argument("Non-rectangular matrix");
    }

    // we allocate memory
    if constexpr (std::is_same_v<T, double>) {
        m_ = gretl_matrix_alloc(rows, cols);
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        m_ = gretl_cmatrix_new(rows, cols);
    } else {
        static_assert(always_false<T>, "from_matrix_(): unknown Container type");
    }
    if (!m_) handle_gretl_error(E_ALLOC);

    // we do the job
    if constexpr (std::is_same_v<T, double>) {
        for (int i=0; i<rows; ++i) {
            for (int j=0; j<cols; ++j) {
                gretl_matrix_set(m_, i, j, data[i][j]);
            }
        }
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        for (int i=0; i<rows; ++i) {
            for (int j=0; j<cols; ++j) {
                const double _Complex z = { data[i][j].real(), data[i][j].imag() };
                gretl_cmatrix_set(m_, i, j, z);
            }
        }
    }
}

//
// public methods
//
gretl_matrix *GretlMatrix::get_ptr (void) const
{
    return m_;
}

double *GretlMatrix::val (void) const
{
    return m_->val;
}

double _Complex *GretlMatrix::z (void) const
{
    if (!is_complex()) throw std::runtime_error("not a complex matrix");

    return m_->z;
}

int GretlMatrix::rows (void) const
{
    return m_->rows;
}

int GretlMatrix::cols (void) const
{
    return m_->cols;
}

[[nodiscard]] bool GretlMatrix::is_complex (void) const noexcept
{
    return m_->is_complex != 0;
}

[[nodiscard]] bool GretlMatrix::is_vector (void) const noexcept
{
    return m_->rows == 1 || m_->cols == 1;
}

std::string GretlMatrix::repr_impl (void)
{
    if (is_complex()) {
        err_ = 0;
        err_ = gretl_cmatrix_print(m_, nullptr, printer_.prn_ptr());
        if (err_) handle_gretl_error(err_);
    } else {
        gretl_matrix_print_to_prn(m_, nullptr, printer_.prn_ptr());
    }

    return printer_.get_buffer();
}

// ROWS == true: we deal we rows of the matrix
template <bool ROWS>
py::list GretlMatrix::rows_cols_names_get (void) const
{
    //
    // method which retrieves rows/cols names
    //
    const char **orig_names;
    int n = 0;

    if (is_complex()) throw std::runtime_error("complex arguments/operands not supported");

    if constexpr (ROWS) {
        orig_names = gretl_matrix_get_rownames(m_);
    } else {
        orig_names = gretl_matrix_get_colnames(m_);
    }
    if (!orig_names) return py::list{};

    if constexpr (ROWS) {
        n = m_->rows;
    } else {
        n = m_->cols;
    }

    return c_style_char_array_to_container<py::list>(orig_names, n);
}
template py::list GretlMatrix::rows_cols_names_get<true>(void) const;
template py::list GretlMatrix::rows_cols_names_get<false>(void) const;

template<bool ROWS>
void GretlMatrix::rows_cols_names_set (const py::list &src) const
{
    //
    // we set rows/cols based on src
    // note: empty src means we erase rows/cols names
    //
    strings_array_ptr names(nullptr, StringsArrayDeleter{});
    int n;

    if (is_complex()) throw std::runtime_error("complex arguments/operands not supported");

    // for empty list, we erase rows/cols names, otherwise we record the length
    if constexpr (ROWS) {
        if (src.empty()) {
            gretl_matrix_set_rownames(m_, nullptr);
            return;
        } else {
            n = m_->rows;
        }
    } else {
        if (src.empty()) {
            gretl_matrix_set_colnames(m_, nullptr);
            return;
        } else {
            n = m_->cols;
        }
    }

    // we allocate char** storage (unique_ptr)
    if (src.size() != static_cast<size_t>(n)) handle_gretl_error(E_INVARG, "names", "supplied list should be of length: " + std::to_string(n));
    char **dst = strings_array_new_with_length(n, OBSLEN);
    if (dst == nullptr) handle_gretl_error(E_ALLOC);
    names.get_deleter().nstrs = n;
    names.reset(dst);

    // we do the job
    for (int i=0; i<n; ++i) {
        std::string s = py::cast<std::string>(src[i]);

        std::strncpy(names.get()[i], s.c_str(), OBSLEN-1);
        names.get()[i][OBSLEN-1] = '\0';
    }
    if constexpr (ROWS) {
        gretl_matrix_set_rownames(m_, names.release());
    } else {
        gretl_matrix_set_colnames(m_, names.release());
    }
}
template void GretlMatrix::rows_cols_names_set<true>(const py::list&) const;
template void GretlMatrix::rows_cols_names_set<false>(const py::list&) const;

void GretlMatrix::set_diagonal (const gretl_matrix *src)
{
    //
    // interface to libgretl's gretl_matrix_set_diagonal()
    //
    if (src == nullptr) throw std::invalid_argument("diagonal must not be empty");
    err_ = gretl_matrix_set_diagonal(m_, src, 0.0);
    if (err_) handle_gretl_error(err_);
}

py::object GretlMatrix::get_element (int i, int j) const
{
    //
    // method for getting matrix element [i, j]
    //
    check_bounds_(i, j);

    if (i < 0) i += m_->rows;
    if (j < 0) j += m_->cols;

    if (is_complex()) {
        return py::cast(c_complex_to_std_complex(gretl_cmatrix_get(m_, i, j)));
    } else {
        return py::cast(gretl_matrix_get(m_, i, j));
    }
}

py::object GretlMatrix::get_element (int i) const
{
    //
    // we get one element of a vector (1D matrix)
    //
    if (!is_vector()) throw py::index_error("Ambiguous matrix index");

    if (is_complex()) {
        return m_->rows == 1 ? get_element(0, i) : get_element(i, 0);
    } else {
        // manual handle negative values (same normalization as set_element(i,j))
        // this is because gretl_vector_get() indexes raw storage with no negative-index support
        if (i < 0) {
            i += (std::max)(m_->rows, m_->cols);
            if (i < 0) handle_gretl_error(E_BOUNDS, "index");
        }
        check_bounds_(i);
        return py::cast(gretl_vector_get(m_, i));
    }
}

GretlMatrixPtr GretlMatrix::get_slice (const py::tuple &slice) const
{
    auto res = exec_slice_(render_slice_(slice));

    return std::make_shared<GretlMatrix>(res.release(), false);
}

GretlMatrixPtr GretlMatrix::get_slice (const py::slice &slice) const
{
    std::string slice_str;
    py::object start = slice.attr("start");
    py::object stop = slice.attr("stop");
    py::object step  = slice.attr("step");

    if (!step.is_none()) {
        if (step.cast<float>() != 1.0) throw std::invalid_argument("slice steps other than 1 are not supported");
    }
    if (is_pynone(start) && is_pynone(stop)) {
        slice_str = ",";
    } else if ((std::min)(m_->rows, m_->cols) > 1) {
        auto cols_slice = py::slice(py::none(), py::none(), py::none());
        py::tuple tuple = py::make_tuple(slice, cols_slice);

        slice_str = render_slice_(tuple);
    } else {
        slice_str = render_slice_(slice);
    }
    auto res = exec_slice_(slice_str);

    return std::make_shared<GretlMatrix>(res.release(), false);
}

GretlMatrixPtr GretlMatrix::get_slice (const std::string &slice) const
{
    auto res = exec_slice_(render_slice_(slice));

    return std::make_shared<GretlMatrix>(res.release(), false);
}

//T: double, std::complex<double>
template <typename T>
void GretlMatrix::set_element (int i, int j, T val)
{
    //
    // method for setting 'val' as matrix element [i, j]
    //
    // manual handle negative values
    // this is because [-1,1] or [-1,-1] produced wrong results
    if (i < 0) {
        i = m_->rows + i;
        if (i < 0) handle_gretl_error(E_BOUNDS, "row index");
    }
    if (j < 0) {
        j = m_->cols + j;
        if (j < 0) handle_gretl_error(E_BOUNDS, "col index");
    }
    check_bounds_(i, j);

    if constexpr (std::is_same_v<T, double>) {
        if (is_complex()) throw std::invalid_argument("matrix is complex, assign a complex number (e.g. 1+2j)");
        gretl_matrix_set(m_, i, j, val);
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        if (!is_complex()) throw std::invalid_argument("matrix is real, use double");
        double _Complex z = { val.real(), val.imag() };

        gretl_cmatrix_set(m_, i, j, z);
    } else {
        static_assert(always_false<T>, "GretlMatrix::set_element(): unknown type");
    }
}
template void GretlMatrix::set_element<double>(int i, int j, double val);
template void GretlMatrix::set_element<std::complex<double>>(int i, int j, std::complex<double> val);

//T: double, std::complex<double>
template <typename T>
void GretlMatrix::set_element (int i, T val)
{
    //
    // method for setting 'val' as vector element [i]
    //
    if (!is_vector()) throw py::index_error("Ambiguous matrix index");

    if constexpr (std::is_same_v<T, double>) {
        if (is_complex()) throw std::invalid_argument("matrix is complex, assign a complex number (e.g. 1+2j)");
        // manual handle negative values (same normalization as set_element(i,j))
        // this is because gretl_vector_set() indexes raw storage with no negative-index support
        if (i < 0) {
            i += (std::max)(m_->rows, m_->cols);
            if (i < 0) handle_gretl_error(E_BOUNDS, "index");
        }
        check_bounds_(i);
        gretl_vector_set(m_, i, val);
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        if (!is_complex()) throw std::invalid_argument("matrix is real, use double");
        m_->rows == 1 ? set_element(0, i, val) : set_element(i, 0, val);
    } else {
        static_assert(always_false<T>, "GretlMatrix::set_element(): unknown type");
    }
}
template void GretlMatrix::set_element<double>(int i, double val);
template void GretlMatrix::set_element<std::complex<double>>(int i, std::complex<double> val);

py::object GretlMatrix::to_numpy() const
{
    //
    // here we construct NumPy as a deep copy of gretl_matrix
    //
    auto res = std::make_unique<GretlTypes>();

    // for complex type
    if (is_complex()) {
        DNumPyArrayZ mat({m_->rows, m_->cols});

        std::memcpy(reinterpret_cast<std::byte*>(mat.mutable_data()),
                    reinterpret_cast<const std::byte*>(m_->z),
                    m_->rows * m_->cols * sizeof(std::complex<double>));
        if ((std::min)(m_->rows, m_->cols) == 1) mat = mat.reshape({mat.size()});
        *res = mat;
    } else {
        DNumPyArrayFC mat({m_->rows, m_->cols});

        std::memcpy(mat.mutable_data(), m_->val, m_->rows * m_->cols * sizeof(double));
        if ((std::min)(m_->rows, m_->cols) == 1) mat = mat.reshape({mat.size()});
        *res = mat;
    }

    return variant_to_pyobject(std::move(*res));
}

py::object GretlMatrix::summary (const py::kwargs& kwargs)
{
    //
    // this function mimics libgretl's 'summary' command for matrix case only
    //
    std::nullptr_t fake_dataset = nullptr;
    py::dict kwargs_copy(kwargs);
    const char *matrix_name = "__gretl4py_matrix_summary";

    // we unpack kwargs at first
    kwargs_copy["matrix"] = matrix_name;
    gretlopt opt = unpack_kwargs_command(SUMMARY, fake_dataset, kwargs_copy);

    // based on 'interact.c'
    err_ = incompatible_options(opt, static_cast<gretlopt>(OPT_B | OPT_W));
    if (err_) handle_gretl_error(err_);
    auto uv = alt_user_var_add(matrix_name, GRETL_TYPE_MATRIX, m_);
    if (!uv) handle_gretl_error(E_ALLOC);
    err_ = matrix_command_driver(SUMMARY, nullptr, matrix_name, nullptr, opt, printer_.prn_ptr());
    user_var_unstack_value(uv);
    if (err_) handle_gretl_error(err_);
    printer_.print();

    return do_get_accessor<GRETL_OBJ_ANY>("$result", fake_dataset, nullptr);
}

GretlMatrixPtr GretlMatrix::neg (void) const
{
    //
    // we return neqation, i.e. (-1 * m_)
    //
    if (is_complex()) return alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::MUL>(-1);

    return alg_dot_operations<double, GretlMatrixAlgOperations::MUL>(-1);
}

// T: GretlMatrix, py::array
// KIND: ADD, SUB, MATMUL, TRUEDIV
template <typename T, GretlMatrixAlgOperations KIND>
GretlMatrixPtr GretlMatrix::alg_operations (const T &rhs) const
{
    //
    // matrices standard operations
    // used as a backend for: __add__, __sub__, __matmul__, __truediv__
    //
    gretl_matrix *rhs_matrix = nullptr;         // right-hand side, used in any kind of operation
    gretl_matrix_ptr rhs_storage{nullptr};      // if we need a temporary unique_ptr to rhs (for py::array)
    gretl_matrix_ptr res{nullptr};

    // we check template parameters
    if constexpr (std::is_same_v<T, GretlMatrix>) {
        rhs_matrix = rhs.get_ptr();
    } else if constexpr (std::is_same_v<T, py::array>) {
        rhs_storage.reset(numpy_to_matrix_(rhs));
        rhs_matrix = rhs_storage.get();
    } else {
        static_assert(always_false<T>, "GretlMatrix::alg_mat_mat_operations(): unknown type");
    }

    // we do the job
    if constexpr (KIND == GretlMatrixAlgOperations::ADD) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_add_sub(m_, rhs_matrix, 1, &err_));
        } else {
            res.reset(gretl_matrix_alloc(m_->rows, m_->cols));
            err_ = gretl_matrix_add(m_, rhs_matrix, res.get());
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::SUB) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_add_sub(m_, rhs_matrix, -1, &err_));
        } else {
            res.reset(gretl_matrix_alloc(m_->rows, m_->cols));
            err_ = gretl_matrix_subtract(m_, rhs_matrix, res.get());
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::MATMUL) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_multiply(m_, rhs_matrix, &err_));
        } else {
            res.reset(gretl_matrix_multiply_new(m_, rhs_matrix, &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::TRUEDIV) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_divide(m_, rhs_matrix, GRETL_MOD_TRANSPOSE, &err_));
        } else {
            res.reset(gretl_matrix_divide(m_, rhs_matrix, GRETL_MOD_TRANSPOSE, &err_));
        }
    } else {
        static_assert(always_false<T>, "Unsupported GretlMatrixAlgOperations value");
    }
    if (err_) handle_gretl_error(err_);

    return std::make_shared<GretlMatrix>(res.release(), false);
}
template GretlMatrixPtr GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::ADD>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::ADD>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::SUB>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::SUB>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::MATMUL>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::MATMUL>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<GretlMatrix, GretlMatrixAlgOperations::TRUEDIV>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_operations<py::array, GretlMatrixAlgOperations::TRUEDIV>(const py::array &rhs) const;

// T: GretlMatrix, py::array, double
// KIND: IADD, ISUB, IMUL, ITRUEDIV, IPOW
template <typename T, GretlMatrixAlgOperations KIND>
GretlMatrix &GretlMatrix::alg_inplace_operations (const T &rhs)
{
    //
    // in-place operations
    // note: here we perform some scalar operations as well
    // used as a backend for: __iadd__, __isub__, __imul__, __itruediv__, __ipow__
    //
    gretl_matrix *rhs_matrix = nullptr;           // right-hand side, used in any kind of operation
    gretl_matrix_ptr rhs_storage{nullptr};        // if we need a temporary unique_ptr to rhs (for py::array)

    // we define scalar-only operations
    constexpr bool scalar_only = (KIND == GretlMatrixAlgOperations::IMUL
    || KIND == GretlMatrixAlgOperations::ITRUEDIV || (KIND == GretlMatrixAlgOperations::IPOW && std::is_same_v<T, double>));

    // we check template parameters
    if constexpr (std::is_same_v<T, GretlMatrix>) {
        rhs_matrix = rhs.get_ptr();
    } else if constexpr (std::is_same_v<T, py::array>) {
        rhs_storage.reset(numpy_to_matrix_(rhs));
        rhs_matrix = rhs_storage.get();
    } else if constexpr (std::is_same_v<T, double>) {
        if constexpr (!scalar_only) {
            rhs_storage.reset(gretl_matrix_alloc(1, 1));
            rhs_matrix = rhs_storage.get();
            gretl_matrix_fill(rhs_matrix, rhs);
        }
    } else {
        static_assert(always_false<T>, "GretlMatrix::alg_mat_mat_inplace_operations(): unknown type");
    }

    // we exclude complex operations
    if constexpr (!scalar_only) {
        if (is_complex() && rhs_matrix->is_complex) {
            handle_gretl_error(E_CMPLX);
        } else if (is_complex() || rhs_matrix->is_complex) {
            handle_gretl_error(E_MIXED);
        }
    }

    // we do the job
    if constexpr (KIND == GretlMatrixAlgOperations::IADD) {
        err_ = gretl_matrix_add_to(m_, rhs_matrix);
    } else if constexpr (KIND == GretlMatrixAlgOperations::ISUB) {
        err_ = gretl_matrix_subtract_from(m_, rhs_matrix);
    } else if constexpr (KIND == GretlMatrixAlgOperations::IMUL && std::is_same_v<T, double>) {
        gretl_matrix_multiply_by_scalar(m_, rhs);
    } else if constexpr (KIND == GretlMatrixAlgOperations::ITRUEDIV && std::is_same_v<T, double>) {
        gretl_matrix_divide_by_scalar(m_, rhs);
    } else if constexpr (KIND == GretlMatrixAlgOperations::IPOW && !std::is_same_v<T, double>) {
        if (rhs_matrix->rows == 1 && rhs_matrix->cols == 1) {
            const double exponent = rhs_matrix->val[0];

            for (int i = 0; i < m_->rows * m_->cols; ++i) {
                m_->val[i] = std::pow(m_->val[i], exponent);
            }
        } else {
            if (m_->rows != rhs_matrix->rows || m_->cols != rhs_matrix->cols) handle_gretl_error(E_NONCONF);
                for (int i = 0; i < m_->rows * m_->cols; ++i) {
                    m_->val[i] = std::pow(m_->val[i], rhs_matrix->val[i]);
                }
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::IPOW && std::is_same_v<T, double>) {
        for (int i=0; i<m_->rows*m_->cols; ++i) {
            m_->val[i] = std::pow(m_->val[i], rhs);
        }
    } else {
        static_assert(always_false<T>, "Unsupported GretlMatrixAlgOperations value");
    }
    if (err_) handle_gretl_error(err_);

    return *this;
}
template GretlMatrix &GretlMatrix::alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::IADD>(const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<py::array, GretlMatrixAlgOperations::IADD>(const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IADD>(const double &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::ISUB>(const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<py::array, GretlMatrixAlgOperations::ISUB>(const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::ISUB>(const double &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IMUL>(const double &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::ITRUEDIV>(const double &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<GretlMatrix, GretlMatrixAlgOperations::IPOW>(const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<py::array, GretlMatrixAlgOperations::IPOW>(const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations<double, GretlMatrixAlgOperations::IPOW>(const double &rhs);

// T: GretlMatrix, py::array, std::complex<double>
// KIND: IADD, ISUB, IMUL, ITRUEDIV
template <typename T, GretlMatrixAlgOperations KIND>
GretlMatrix &GretlMatrix::alg_inplace_operations_complex (const T &rhs)
{
    //
    // here we implement non-existing in libgretl inplace operations for complex matrices
    // used as a backend for: __iadd__, __isub__, __imul__, __itruediv__, __ipow__
    //
    gretl_matrix *rhs_matrix = nullptr;           // right-hand side, used in any kind of operation
    gretl_matrix_ptr rhs_storage{nullptr};        // if we need a temporary unique_ptr to rhs (for py::array)
    double _Complex z;

    // we check template parameters
    if constexpr (std::is_same_v<T, GretlMatrix>) {
        rhs_matrix = rhs.get_ptr();
    } else if constexpr (std::is_same_v<T, py::array>) {
        rhs_storage.reset(numpy_to_matrix_(rhs));
        rhs_matrix = rhs_storage.get();
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        if constexpr (KIND != GretlMatrixAlgOperations::IPOW) {
            z = { rhs.real(), rhs.imag() };
        }
    } else {
        static_assert(always_false<T>, "GretlMatrix::alg_inplace_operations_complex(): unknown type");
    }

    // we check matrices features
    if (!is_complex()) throw std::invalid_argument("left matrix is real, should be complex");
    if constexpr (std::is_same_v<T, GretlMatrix> || std::is_same_v<T, py::array>) {
        if (!rhs_matrix->is_complex) throw std::invalid_argument("right matrix is real, should be complex");
        if (m_->rows != rhs_matrix->rows || m_->cols != rhs_matrix->cols) handle_gretl_error(E_NONCONF);
    }

    // we do the job
    for (int i=0; i<m_->rows * m_->cols; ++i) {
        if constexpr (KIND == GretlMatrixAlgOperations::IADD) {
            if constexpr (std::is_same_v<T, std::complex<double>>) {
                m_->z[i] += z;
            } else {
                m_->z[i] += rhs_matrix->z[i];
            }
        } else if constexpr (KIND == GretlMatrixAlgOperations::ISUB) {
            if constexpr (std::is_same_v<T, std::complex<double>>) {
                m_->z[i] -= z;
            } else {
                m_->z[i] -= rhs_matrix->z[i];
            }
        } else if constexpr (KIND == GretlMatrixAlgOperations::IMUL) {
            if constexpr (std::is_same_v<T, std::complex<double>>) {
                m_->z[i] *= z;
            } else {
                static_assert(always_false<T>, "GretlMatrix::alg_inplace_operations_complex(): unsupported type");
            }
        } else if constexpr (KIND == GretlMatrixAlgOperations::ITRUEDIV) {
            if constexpr (std::is_same_v<T, std::complex<double>>) {
                m_->z[i] /= z;
            } else {
                static_assert(always_false<T>, "GretlMatrix::alg_inplace_operations_complex(): unsupported type");
            }
        } else if constexpr (KIND == GretlMatrixAlgOperations::IPOW) {
            auto base = c_complex_to_std_complex(m_->z[i]);

            if constexpr (std::is_same_v<T, std::complex<double>>) {
                m_->z[i] = std_complex_to_c_complex(std::pow(base, rhs));
            } else {
                m_->z[i] = std_complex_to_c_complex(std::pow(base, c_complex_to_std_complex(rhs_matrix->z[i])));
            }
        }
    }

    return *this;
}
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::IADD> (const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::IADD> (const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IADD> (const std::complex<double> &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::ISUB> (const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::ISUB> (const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::ISUB> (const std::complex<double> &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IMUL> (const std::complex<double> &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::ITRUEDIV> (const std::complex<double> &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<GretlMatrix, GretlMatrixAlgOperations::IPOW> (const GretlMatrix &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<py::array, GretlMatrixAlgOperations::IPOW> (const py::array &rhs);
template GretlMatrix &GretlMatrix::alg_inplace_operations_complex<std::complex<double>, GretlMatrixAlgOperations::IPOW> (const std::complex<double> &rhs);

// T: GretlMatrix or py::array, double, std::complex<double>>
// KIND: ADD, SUB, MUL, TRUEDIV, POW
template <typename T, GretlMatrixAlgOperations KIND>
GretlMatrixPtr GretlMatrix::alg_dot_operations (const T &rhs) const
{
    //
    // matrices element-wise (libgretl's 'dot') operations
    // note: here we perform some scalar operations as well
    // used as a backend for: __add__, __sub__, __mul__, __truediv__, __pow__
    //
    gretl_matrix *rhs_matrix = nullptr;           // right-hand side, used in any kind of operation
    gretl_matrix_ptr rhs_storage{nullptr};        // if we need a temporary unique_ptr to rhs (for py::array)
    gretl_matrix_ptr res{nullptr};

    // we check template parameters
    if constexpr (std::is_same_v<T, GretlMatrix>) {
        rhs_matrix = rhs.get_ptr();
    } else if constexpr (std::is_same_v<T, py::array>) {
        rhs_storage.reset(numpy_to_matrix_(rhs));
        rhs_matrix = rhs_storage.get();
    } else if constexpr (std::is_same_v<T, double>) {
        rhs_storage.reset(gretl_matrix_alloc(1, 1));
        if (!rhs_storage) handle_gretl_error(E_ALLOC);

        rhs_matrix = rhs_storage.get();
        gretl_matrix_set(rhs_matrix, 0, 0, rhs);
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        double _Complex z = { rhs.real(), rhs.imag() };

        rhs_storage.reset(gretl_cmatrix_new(1, 1));
        if (!rhs_storage) handle_gretl_error(E_ALLOC);

        rhs_matrix = rhs_storage.get();
        gretl_cmatrix_set(rhs_matrix, 0, 0, z);
    } else {
        static_assert(always_false<T>, "GretlMatrix::alg_mat_mat_dot_operations(): unknown type");
    }

    // we do the job
    if constexpr (KIND == GretlMatrixAlgOperations::ADD) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '+', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '+', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::SUB) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '-', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '-', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::MUL) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '*', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '*', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::TRUEDIV) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '/', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '/', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::POW) {
        //(has_negative_base(m_) && has_non_integer_exponent(rhs_matrix))
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '^', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '^', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::EQ) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '=', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '=', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::NE) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '!', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '!', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::LT) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '<', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '<', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::LE) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '[', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '[', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::GT) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, '>', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, '>', &err_));
        }
    } else if constexpr (KIND == GretlMatrixAlgOperations::GE) {
        if (is_complex() || rhs_matrix->is_complex) {
            res.reset(gretl_cmatrix_dot_op(m_, rhs_matrix, ']', &err_));
        } else {
            res.reset(gretl_matrix_dot_op(m_, rhs_matrix, ']', &err_));
        }
    } else {
        static_assert(always_false<T>, "Unsupported GretlMatrixAlgOperations value");
    }
    if (err_) handle_gretl_error(err_);

    return std::make_shared<GretlMatrix>(res.release(), false);
}
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::ADD>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::ADD>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::SUB>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::SUB>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::MUL>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::MUL>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::MUL>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::MUL>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::TRUEDIV>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::TRUEDIV>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::POW>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::POW>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::POW>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::POW>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::EQ>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::EQ>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::EQ>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::EQ>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::NE>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::NE>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::NE>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::NE>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::LT>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::LT>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::LT>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::LT>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::LE>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::LE>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::LE>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::LE>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::GT>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::GT>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::GT>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::GT>(const std::complex<double> &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<GretlMatrix, GretlMatrixAlgOperations::GE>(const GretlMatrix &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<py::array, GretlMatrixAlgOperations::GE>(const py::array &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<double, GretlMatrixAlgOperations::GE>(const double &rhs) const;
template GretlMatrixPtr GretlMatrix::alg_dot_operations<std::complex<double>, GretlMatrixAlgOperations::GE>(const std::complex<double> &rhs) const;
