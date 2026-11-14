"""
GretlBundle regression tests - covers the double-free regression found while
fixing the original leak (round-2, finding #07) and the missing null-check
after gretl_matrix_alloc() in set_element<py::array>.
"""
import copy
import resource

import numpy as np
import pytest

import gretl


def _rss_kb():
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss


def test_basic_bundle():
    b = gretl.Bundle(x=1, y=2.5, s="hi")
    assert b.to_dict() == {"x": 1, "y": 2.5, "s": "hi"}


def test_nested_bundle_to_dict_no_double_free():
    # extract_() used to copy-construct a temporary GretlBundle sharing the
    # same underlying pointer as the live object, double-freeing it once the
    # leak (free_b_ always false) was fixed. Repeated calls here would
    # previously corrupt the heap.
    b = gretl.Bundle()
    b["inner"] = gretl.Bundle(x=1)
    for _ in range(20_000):
        d = b.to_dict()
    assert d == {"inner": {"x": 1}}
    assert b["inner"].to_dict() == {"x": 1}  # still readable afterward


def test_nested_bundle_no_leak():
    b = gretl.Bundle()
    b["inner"] = gretl.Bundle(x=1)
    before = _rss_kb()
    for _ in range(20_000):
        b.to_dict()
    after = _rss_kb()
    assert after - before < 20_000, f"RSS grew by {after - before} KB across 20k to_dict() calls"


def test_copy_and_deepcopy_protocol():
    b = gretl.Bundle(x=1)
    c1 = copy.copy(b)
    c2 = copy.deepcopy(b)
    c1["x"] = 2
    assert b.to_dict() == {"x": 1}
    assert c1.to_dict() == {"x": 2}
    assert c2.to_dict() == {"x": 1}


def test_matrix_from_array_element():
    # regression for the missing null-check after gretl_matrix_alloc() in
    # set_element<py::array> - this is the normal-path correctness check;
    # the actual null branch can only be hit under real OOM.
    b = gretl.Bundle()
    b["m"] = np.array([[1.0, 2.0], [3.0, 4.0]])
    assert b.to_dict()["m"].numpy_view.tolist() == [[1.0, 2.0], [3.0, 4.0]]


# --- full set_element type coverage -----------------------------------------
#
# __setitem__ has one overload per type (bool/int/double/string/array/
# GretlBundlePtr/GretlMatrixPtr) - each is its own code path in
# gretl4py_bundle.cpp. Only the array path had a known bug; exercising the
# rest here is pedantic coverage, not known-bug regression.

def test_all_scalar_types_roundtrip():
    b = gretl.Bundle()
    b["b"] = True
    b["i"] = 42
    b["d"] = 3.14
    b["s"] = "hello"
    d = b.to_dict()
    assert d["b"] is True or d["b"] == 1  # bool surfaces as int in hansl bundles
    assert d["i"] == 42
    assert d["d"] == 3.14
    assert d["s"] == "hello"


def test_matrix_object_element_distinct_from_array_path():
    # b["m"] = gretl.Matrix(...) is gretl_bundle_set_data(..., t->get_ptr())
    # - a genuinely different code path than b["m"] = np.array(...), which
    # goes through NumPyArray_extract + gretl_matrix_alloc + donate_data.
    # Uses a genuine 2x2 (not 1xN) shape deliberately - a single-row/column
    # matrix is_vector() and numpy_view flattens it to 1-D, which isn't
    # what this test is checking.
    b = gretl.Bundle()
    b["m"] = gretl.Matrix(np.array([[5.0, 6.0], [7.0, 8.0]]))
    assert b.to_dict()["m"].numpy_view.tolist() == [[5.0, 6.0], [7.0, 8.0]]


def test_key_overwrite():
    b = gretl.Bundle(x=1)
    b["x"] = 2
    assert b.to_dict() == {"x": 2}


def test_delete_and_pop():
    b = gretl.Bundle(x=1, y=2)
    del b["x"]
    assert "x" not in b.to_dict()
    popped = b.pop("y")
    assert popped == 2
    assert b.to_dict() == {}


def test_to_dict_does_not_return_stale_deleted_keys():
    # found while writing this suite, not a previously-known bug: to_dict()
    # reuses a persistent as_dict_ member and never cleared it before
    # repopulating, so a key deleted from the bundle survived forever in
    # every *subsequent* to_dict() call - to_dict() would keep returning
    # {'x': 1, 'y': 2} even after both keys were deleted.
    b = gretl.Bundle(x=1, y=2)
    assert b.to_dict() == {"x": 1, "y": 2}
    del b["x"]
    assert b.to_dict() == {"y": 2}
    del b["y"]
    assert b.to_dict() == {}


def test_keys():
    b = gretl.Bundle(x=1, y=2)
    assert sorted(b.keys()) == ["x", "y"]


def test_empty_bundle():
    b = gretl.Bundle()
    assert b.to_dict() == {}
    assert b.keys() == []


def test_deeply_nested_bundles():
    b = gretl.Bundle()
    b["l1"] = gretl.Bundle()
    b["l1"].to_dict()  # sanity: readable
    inner = gretl.Bundle()
    inner["l2"] = gretl.Bundle(leaf=1)
    b["l1"] = inner
    d = b.to_dict()
    assert d == {"l1": {"l2": {"leaf": 1}}}


def test_numpy_donation_invalid_key():
    # Regression: mat.release() before donate_data() could
    # leak the allocated matrix when the operation failed.
    b = gretl.Bundle()

    for _ in range(1000):
        with pytest.raises(Exception):
            b[""] = np.ones((4, 4))


def test_numpy_donation_replacement():
    # Repeatedly replace an existing donated matrix.
    b = gretl.Bundle()

    for i in range(1000):
        b["m"] = np.full((4, 4), float(i))

    np.testing.assert_array_equal(
        b["m"].numpy_view,
        np.full((4, 4), 999.0)
    )


def test_pop_matrix_lifetime():
    # pop() must return a matrix independent of the bundle.
    b = gretl.Bundle()
    b["m"] = np.arange(9.0).reshape(3, 3)

    m = b.pop("m")
    del b

    np.testing.assert_array_equal(
        m.numpy_view,
        np.arange(9.0).reshape(3, 3)
    )


def test_bundle_matrix_copy_independence():
    # get_element() must copy the underlying gretl_matrix.
    b = gretl.Bundle()
    x = np.arange(9.0).reshape(3, 3)

    b["m"] = x
    x[:] = -1

    np.testing.assert_array_equal(
        b["m"].numpy_view,
        np.arange(9.0).reshape(3, 3)
    )

