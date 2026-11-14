"""
Property-based (hypothesis) tests for GretlMatrix indexing.

Unlike the rest of the suite, these aren't regression tests for a specific
known bug - they're a systematic sweep over shapes/indices, the same kind of
exploration that would have found the INT_MIN and negative-index bugs
without anyone having to think of those exact values by hand. If a *new*
indexing bug gets introduced later, this is the file most likely to catch
it before a person does.
"""
import numpy as np
import pytest
from hypothesis import given, example, settings, strategies as st

import gretl

INT_MIN = -2147483648
INT_MAX = 2147483647


# --- 2-arg (matrix) indexing -------------------------------------------------

@settings(max_examples=300)
@given(
    rows=st.integers(1, 8),
    cols=st.integers(1, 8),
    i=st.integers(-20, 20),
    j=st.integers(-20, 20),
)
@example(rows=1, cols=1, i=0, j=0)
@example(rows=3, cols=3, i=INT_MIN, j=0)
@example(rows=3, cols=3, i=0, j=INT_MIN)
@example(rows=3, cols=3, i=INT_MAX, j=0)
def test_matrix_set_index_in_or_out_of_range(rows, cols, i, j):
    m = gretl.Matrix.zeros(rows, cols)
    in_range = (-rows <= i < rows) and (-cols <= j < cols)

    if in_range:
        m[i, j] = 7.0
        # canonicalize to non-negative form and confirm it lands on the
        # same element (get_element(non-negative) always returns a plain
        # float, sidestepping the pre-existing quirk where a *negative*
        # 2-arg read returns a 1x1-matrix-wrapped value instead)
        pi = i if i >= 0 else i + rows
        pj = j if j >= 0 else j + cols
        assert m[pi, pj] == 7.0
    else:
        with pytest.raises(IndexError):
            m[i, j] = 7.0


@settings(max_examples=300)
@given(
    rows=st.integers(1, 8),
    cols=st.integers(1, 8),
    i=st.integers(-20, 20),
    j=st.integers(-20, 20),
)
@example(rows=3, cols=3, i=INT_MIN, j=0)
@example(rows=3, cols=3, i=0, j=INT_MIN)
def test_matrix_get_index_in_or_out_of_range(rows, cols, i, j):
    m = gretl.Matrix.zeros(rows, cols)
    in_range = (-rows <= i < rows) and (-cols <= j < cols)

    if in_range:
        m[i, j]  # must not raise; the exact return shape/type isn't asserted
    else:
        with pytest.raises(IndexError):
            m[i, j]


# --- 1-arg (vector) indexing -------------------------------------------------

@settings(max_examples=300)
@given(n=st.integers(1, 20), i=st.integers(-40, 40))
@example(n=5, i=INT_MIN)
@example(n=5, i=INT_MAX)
@example(n=1, i=-1)
def test_vector_set_index_in_or_out_of_range(n, i):
    v = gretl.Matrix.zeros(1, n)
    in_range = -n <= i < n

    if in_range:
        v[i] = 3.0
        pi = i if i >= 0 else i + n
        assert v[pi] == 3.0
    else:
        with pytest.raises(IndexError):
            v[i] = 3.0


@settings(max_examples=300)
@given(n=st.integers(1, 20), i=st.integers(-40, 40))
@example(n=5, i=INT_MIN)
def test_vector_get_index_in_or_out_of_range(n, i):
    v = gretl.Matrix.zeros(1, n)
    in_range = -n <= i < n

    if in_range:
        v[i]
    else:
        with pytest.raises(IndexError):
            v[i]


# --- construction round-trips (values survive NumPy -> GretlMatrix -> NumPy)-

@settings(max_examples=100)
@given(
    data=st.lists(
        st.lists(st.floats(allow_nan=False, allow_infinity=False, width=32), min_size=1, max_size=6),
        min_size=1, max_size=6,
    ).filter(lambda rows: len(set(len(r) for r in rows)) == 1)  # rectangular
)
def test_matrix_construction_roundtrip(data):
    arr = np.array(data, dtype=np.float64)
    m = gretl.Matrix(arr)
    assert m.rows == arr.shape[0]
    assert m.cols == arr.shape[1]
    # element-wise, not via .numpy_view: a 1xN/Nx1 shape is_vector() and
    # numpy_view flattens it to 1-D, which would make a shape-based
    # comparison fail for reasons unrelated to what this test checks
    for i in range(arr.shape[0]):
        for j in range(arr.shape[1]):
            assert m[i, j] == arr[i, j]
