"""
Dataset API regression and smoke tests from the October 2026 audit.

These tests intentionally use the real denmark dataset.  They cover verified
Python signatures, overloads, result shapes, and error paths.  The kmeans
lifetime checks are crash regressions, not proof of leak freedom.

Keep the tests independent: every mutating test uses a fresh dataset or clone.
"""
import gc

import numpy as np
import pytest

import gretl


@pytest.fixture
def denmark():
    return gretl.get_data("denmark", quiet=True)


def _values(matrix):
    """Gretl vectors may expose a 1-D NumPy view."""
    return np.asarray(matrix.to_numpy()).ravel()


def test_clone_independent_setobs(denmark):
    original = list(denmark.varnames)
    clone = denmark.clone()
    clone.setobs("4", "2000:1")
    assert clone.to_matrix(["LRM"]).rows == 55
    clone.setobs("1", "1")
    assert clone.to_matrix(["LRM"]).rows == 55
    assert list(denmark.varnames) == original


@pytest.mark.parametrize("varlist", ["LRM LRY", ["LRM", "LRY"]])
def test_lags_generate_four_series(denmark, varlist):
    before = set(denmark.varnames)
    denmark.lags(varlist, 2)
    assert set(denmark.varnames) - before == {
        "LRM_1", "LRM_2", "LRY_1", "LRY_2"
    }


def test_lags_invalid_order_rejected(denmark):
    with pytest.raises(Exception, match="Invalid lag order"):
        denmark.lags("LRM", -1)


@pytest.mark.parametrize("method,descending", [
    ("sortby", False), ("dsortby", True)
])
@pytest.mark.parametrize("as_list", [False, True])
def test_sortby_undated(denmark, method, descending, as_list):
    data = denmark.clone()
    data.setobs("1", "1")
    argument = ["LRM"] if as_list else "LRM"
    getattr(data, method)(argument)
    differences = np.diff(_values(data.to_matrix(["LRM"])))
    assert np.all(differences <= 0 if descending else differences >= 0)


def test_sortby_rejects_time_series(denmark):
    with pytest.raises(Exception, match="undated"):
        denmark.sortby("LRM")


def test_addobs(denmark):
    data = denmark.clone()
    n = data.to_matrix(["LRM"]).rows
    data.addobs(5)
    assert data.to_matrix(["LRM"]).rows == n + 5


def test_expand_annual_to_quarterly(denmark):
    data = denmark.clone()
    data.setobs("1", "2000")
    n = data.to_matrix(["LRM"]).rows
    data.expand()
    assert data.to_matrix(["LRM"]).rows == 4 * n


@pytest.mark.parametrize("varlist", ["LRM LRY", ["LRM", "LRY"]])
def test_corr_and_cov_overloads(denmark, varlist):
    corr = denmark.corr(varlist)
    cov = denmark.cov(varlist)
    assert (corr.rows, corr.cols) == (2, 2)
    assert (cov.rows, cov.cols) == (2, 2)
    np.testing.assert_allclose(
        np.asarray(corr.to_numpy()).reshape(2, 2)[0, 1],
        0.868474, atol=1e-5
    )


@pytest.mark.parametrize("varlist", ["LRM LRY", ["LRM", "LRY"]])
def test_xcorrgm(denmark, varlist):
    result = denmark.xcorrgm(varlist, 4)
    assert (result.rows, result.cols) == (9, 2)


@pytest.mark.parametrize("varlist", [
    "LRM LRY IBO", ["LRM", "LRY", "IBO"]
])
def test_pca(denmark, varlist):
    result = denmark.pca(varlist)
    assert isinstance(result, gretl.Bundle)
    data = result.to_dict()
    assert "eigenvals" in data or "eigenvalues" in data or len(data) > 0


@pytest.mark.parametrize("varlist", [
    "LRM LRY IBO", ["LRM", "LRY", "IBO"]
])
def test_kmeans_with_cluster_count(denmark, varlist):
    result = denmark.kmeans(varlist, 3)
    assert isinstance(result, gretl.Bundle)
    values = result.to_dict()
    assert "centroids" in values and "cluster_id" in values
    assert values["cluster_id"].rows == 55
    assert values["centroids"].rows == 3


@pytest.mark.parametrize("varlist", [
    "LRM LRY IBO", ["LRM", "LRY", "IBO"]
])
def test_kmeans_with_initial_centers(denmark, varlist):
    X = np.asarray(denmark.to_matrix(["LRM", "LRY", "IBO"]).to_numpy())
    centers = gretl.Matrix(np.asfortranarray(X[[0, 20, 40], :]))
    result = denmark.kmeans(varlist, centers)
    values = result.to_dict()
    assert values["init"] == "user-specified"
    assert values["centroids"].rows == 3


def test_kmeans_owned_bundle_lifetime(denmark):
    """Regression for copy=false: adopted gretl_bundle must be destroyed."""
    for _ in range(40):
        result = denmark.kmeans("LRM LRY IBO", 3)
        assert result.to_dict()["cluster_id"].rows == 55
        del result
    gc.collect()


@pytest.mark.parametrize("varlist", [
    "group1 group2", ["group1", "group2"]
])
def test_xtab_and_result_matrix(denmark, varlist):
    denmark.new_series("group1 = (LRM > mean(LRM))")
    denmark.new_series("group2 = (LRY > mean(LRY))")
    result = denmark.xtab(varlist)
    assert isinstance(result, gretl.Matrix)
    assert (result.rows, result.cols) == (3, 3)
    table = np.asarray(result.to_numpy()).reshape(3, 3)
    np.testing.assert_array_equal(
        table, [[26, 8, 34], [5, 16, 21], [31, 24, 55]]
    )


def test_to_matrix_rejects_invalid_observation_range(denmark):
    # startobs/endobs are zero-based observation indices.
    with pytest.raises(Exception):
        denmark.to_matrix(["LRM"], startobs=-1)
    with pytest.raises(Exception):
        denmark.to_matrix(["LRM"], endobs=55)
    with pytest.raises(Exception):
        denmark.to_matrix(["LRM"], startobs=10, endobs=5)


def test_sample_and_restore(denmark):
    denmark.sample("1975:1", "1980:4")
    denmark.sample(full=True)
    assert denmark.to_matrix(["LRM"]).rows == 55
