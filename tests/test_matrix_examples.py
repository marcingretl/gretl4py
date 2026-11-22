"""Matrix interop and operator regressions adapted from demo/matrices.py.

Index boundary checks, 0-D constructors, copy/deepcopy and basic slicing
already live in test_matrix.py and test_matrix_properties.py. Keep these
checks focused on previously uncovered Python/NumPy/pandas interactions.
"""

import numpy as np
import pytest

import gretl


def _array(matrix):
    return np.asarray(matrix.numpy_view)


def test_matrix_row_and_column_names_roundtrip():
    m = gretl.Matrix(np.array([[1., 2., 3.], [4., 5., 6.]]))
    m.rnameset(["first", "second"])
    m.cnameset(["one", "two", "three"])
    assert list(m.rnameget()) == ["first", "second"]
    assert list(m.cnameget()) == ["one", "two", "three"]


def test_numpy_view_aliases_matrix_but_to_numpy_is_a_copy():
    m = gretl.Matrix(np.arange(1., 7.).reshape(2, 3))
    view = m.numpy_view
    saved = m.to_numpy()
    initial = np.array(saved, copy=True)
    # Mutations in either direction propagate through the shared view.
    m[0, 0] = 40.0
    assert view[0, 0] == 40.0
    view[1, 1] = 50.0
    assert m[1, 1] == 50.0
    # A materialized copy must keep the original values.
    np.testing.assert_array_equal(saved, initial)
    saved[0, 0] = -1.0
    assert m[0, 0] == 40.0


def test_complex_numpy_constructor_and_matmul():
    left = np.array([[1 + 2j, 2 - 1j], [3 + 0j, -1 + 1j]])
    right = np.array([[2 + 1j, 1j], [1 - 2j, 3 + 0j]])
    a = gretl.Matrix(left)
    b = gretl.Matrix(right)
    np.testing.assert_allclose(_array(a @ b), left @ right)


def test_matrix_elementwise_power_with_matrix_and_numpy_view():
    a = gretl.Matrix(np.array([[2., 3.], [4., 5.]]))
    b = gretl.Matrix(np.array([[2., 1.], [0., 3.]]))
    expected = np.array([[4., 3.], [1., 125.]])
    np.testing.assert_allclose(_array(a ** b), expected)
    np.testing.assert_allclose(_array(a ** b.numpy_view), expected)


def test_inplace_numpy_view_arithmetic():
    a = gretl.Matrix(np.array([[1., 2.], [3., 4.]]))
    b = gretl.Matrix(np.array([[0.5, 1.5], [2.5, 3.5]]))
    a += b.numpy_view
    np.testing.assert_allclose(_array(a), [[1.5, 3.5], [5.5, 7.5]])
    a -= b.numpy_view
    np.testing.assert_allclose(_array(a), [[1., 2.], [3., 4.]])


def test_reflected_scalar_operations():
    a = gretl.Matrix(np.array([[1., 2.], [4., 5.]]))
    np.testing.assert_allclose(_array(2 - a), [[1., 0.], [-2., -3.]])
    np.testing.assert_allclose(_array(10 / a), [[10., 5.], [2.5, 2.]])
    np.testing.assert_allclose(_array(2 * a), [[2., 4.], [8., 10.]])


def test_reflected_complex_scalar_division():
    values = np.array([[1 + 2j, 2 - 1j],
                       [3 + 0j, 4 + 1j]])
    a = gretl.Matrix(values)

    np.testing.assert_allclose(
        _array((2 + 3j) / a),
        (2 + 3j) / values,
    )


def test_complex_scalar_inplace_arithmetic():
    a = gretl.Matrix(np.array([[1 + 2j, 2 - 1j], [3 + 0j, 4 + 1j]]))
    reference = np.array([[1 + 2j, 2 - 1j], [3 + 0j, 4 + 1j]])
    a += 1 + 0.5j
    reference += 1 + 0.5j
    a *= 2 - 1j
    reference *= 2 - 1j
    np.testing.assert_allclose(_array(a), reference)


def test_negative_slice_endpoints_in_two_dimensions():
    m = gretl.Matrix(np.arange(1., 13.).reshape(3, 4))
    result = m[-2:, 1:-1]
    np.testing.assert_array_equal(_array(result), [[6., 7.], [10., 11.]])


def test_nonunit_slice_step_is_rejected():
    # gretl4py's matrix slicing contract currently does not support strides.
    m = gretl.Matrix(np.arange(1., 13.).reshape(3, 4))
    with pytest.raises((TypeError, ValueError, RuntimeError)):
        _ = m[::2, :]


def test_pandas_copy_matches_matrix_values():
    pd = pytest.importorskip("pandas")
    m = gretl.Matrix(np.array([[1., 2., 3.], [4., 5., 6.]]))
    df = m.to_pandas()
    assert isinstance(df, pd.DataFrame)
    np.testing.assert_array_equal(df.to_numpy(), [[1., 2., 3.], [4., 5., 6.]])
    # Do not assume pandas_view is writable across pandas versions/layouts.
    df.iloc[0, 0] = -7.0
    assert m[0, 0] == 1.0
