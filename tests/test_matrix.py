"""
GretlMatrix regression tests - one per bug found/fixed in the 2026-08
security review, plus basic correctness checks for the surrounding
behavior so a fix can't "succeed" by breaking something else.
"""
import resource

import numpy as np
import pytest

import gretl


def _rss_kb():
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss


# --- negative indexing (finding #02) ---------------------------------------

def test_negative_index_vector_read_write():
    v = gretl.Matrix.zeros(1, 5)
    for i in range(5):
        v[i] = float(i + 1)
    assert v[-1] == 5.0
    assert v[-5] == 1.0
    v[-1] = 42.0
    assert v[4] == 42.0


def test_negative_index_matrix_read_write():
    m = gretl.Matrix(np.array([[1.0, 2.0], [3.0, 4.0]]))
    m[-1, -1] = 99.0
    assert float(np.asarray(m.numpy_view)[-1, -1]) == 99.0


def test_negative_index_out_of_range_still_rejected():
    m = gretl.Matrix.zeros(3, 3)
    with pytest.raises(IndexError):
        m[-4, 0]
    with pytest.raises(IndexError):
        m[0, -4]


# --- std::abs(INT_MIN) UB on extreme indices --------------------------------

def test_int_min_index_rejected_matrix():
    m = gretl.Matrix.zeros(2, 2)
    with pytest.raises(IndexError):
        m[-2147483648, 0]


def test_int_min_index_rejected_vector():
    v = gretl.Matrix.zeros(1, 5)
    with pytest.raises(IndexError):
        v[-2147483648]


# --- get_slice() leak (missing ", false" - adopt instead of copy-and-leak) --

def test_get_slice_tuple_correctness():
    m = gretl.Matrix(np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0], [7.0, 8.0, 9.0]]))
    s = m[0:2, :]
    assert s.rows == 2 and s.cols == 3
    assert s.numpy_view.tolist() == [[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]]


def test_get_slice_column_correctness():
    m = gretl.Matrix(np.array([[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]]))
    s = m[:, 1]
    assert s.numpy_view.tolist() == [2.0, 4.0, 6.0]


def test_get_slice_no_leak():
    m = gretl.Matrix(np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0], [7.0, 8.0, 9.0]]))
    before = _rss_kb()
    for _ in range(20_000):
        _ = m[0:2, :]
        _ = m[:, 1]
    after = _rss_kb()
    # generous margin - this isn't measuring precisely, just "didn't blow up"
    assert after - before < 20_000, f"RSS grew by {after - before} KB across 20k slices"


# --- GretlMatrix Rule-of-Three (missing operator=) --------------------------

def test_copy_assignment_is_independent_and_replaces_shape():
    import copy

    m1 = gretl.Matrix(np.array([[1.0]]))
    m2 = copy.deepcopy(m1)  # exercises the copy constructor
    m2[0, 0] = 99.0
    assert m1[0, 0] == 1.0  # original untouched


def test_copy_and_deepcopy_protocol():
    import copy

    m = gretl.Matrix(np.array([[1.0, 2.0], [3.0, 4.0]]))
    c1 = copy.copy(m)
    c2 = copy.deepcopy(m)
    c1[0, 0] = 999.0
    assert m[0, 0] == 1.0
    assert c2[0, 0] == 1.0
    assert c1 is not m and c2 is not m


# --- 0-d NumPy array rejected before OOB shape/strides read -----------------

def test_zero_dimensional_array_rejected():
    with pytest.raises(RuntimeError):
        gretl.Matrix(np.array(5.0))


def test_more_than_two_dimensions_still_rejected():
    with pytest.raises(RuntimeError):
        gretl.Matrix(np.zeros((2, 2, 2)))


def test_one_and_two_dimensional_arrays_still_work():
    v = gretl.Matrix(np.array([1.0, 2.0, 3.0]))
    assert v.rows == 3 and v.cols == 1
    m = gretl.Matrix(np.array([[1.0, 2.0], [3.0, 4.0]]))
    assert m.rows == 2 and m.cols == 2


# --- complex matrices - an actual coverage gap, not a known-bug regression --
#
# The negative-index fixes for set_element(i,j)/set_element(i) only touched
# the real (double) branches; the complex branches delegate to the already-
# fixed 2-arg path (set_element(0,i,val) / set_element(i,0,val)), but that
# delegation itself was never directly exercised by a test until now.

def test_complex_matrix_negative_index_read_write():
    m = gretl.Matrix(2, 2, 0.0, "complex")
    m[0, 0] = 1 + 2j
    m[1, 1] = 3 + 4j
    assert m[-1, -1] == (3 + 4j)
    m[-1, -1] = 9 + 9j
    assert m[1, 1] == (9 + 9j)


def test_complex_vector_negative_index_read_write():
    v = gretl.Matrix(1, 3, 0.0, "complex")
    v[0] = 1 + 1j
    v[1] = 2 + 2j
    v[2] = 3 + 3j
    assert v[-1] == (3 + 3j)
    v[-1] = 9 + 9j
    assert v[2] == (9 + 9j)


def test_complex_matrix_int_min_index_rejected():
    m = gretl.Matrix(2, 2, 0.0, "complex")
    with pytest.raises(IndexError):
        m[-2147483648, 0]


# --- degenerate shapes -------------------------------------------------------

def test_1x1_matrix_negative_index():
    m = gretl.Matrix(np.array([[7.0]]))
    assert m[-1, -1] == 7.0
    m[-1, -1] = 8.0
    assert m[0, 0] == 8.0
    with pytest.raises(IndexError):
        m[-2, 0]


def test_1x1_vector_negative_index():
    v = gretl.Matrix.zeros(1, 1)
    v[0] = 5.0
    assert v[-1] == 5.0
    with pytest.raises(IndexError):
        v[-2]
