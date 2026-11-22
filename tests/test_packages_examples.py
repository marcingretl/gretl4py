"""Additional contributed-package API tests from demo/packages.py.

No installation or download is triggered by these tests. The baseline uses
only the criteria package already exercised by tests/conftest.py. Other
packages are opt-in, to avoid slow or environment-dependent default runs.
"""

import os

import numpy as np
import pytest

import gretl


# Explicit opt-in; no expensive Bayesian sampling in a standard pytest run.
_OPTIONAL = os.getenv("GRETL4PY_RUN_OPTIONAL_PACKAGES") == "1"


def test_criteria_keyword_argument_order(criteria_package):
    """Argument order is irrelevant when dispatching a package function."""
    left = criteria_package._function_exec(
        "criteria", ess=100.0, n=50, k=3, quiet=1
    )
    right = criteria_package._function_exec(
        "criteria", k=3, quiet=1, n=50, ess=100.0
    )
    np.testing.assert_allclose(
        np.asarray(left.numpy_view), np.asarray(right.numpy_view), rtol=0, atol=0
    )
    assert left.rows == 1 and left.cols == 4


def test_criteria_published_function_dispatch(criteria_package):
    """An included function is callable through the gretl module facade."""
    via_facade = gretl.criteria(ess=100.0, n=50, k=3, quiet=1)
    via_package = criteria_package._function_exec(
        "criteria", ess=100.0, n=50, k=3, quiet=1
    )
    np.testing.assert_allclose(
        np.asarray(via_facade.numpy_view),
        np.asarray(via_package.numpy_view), rtol=0, atol=0
    )


@pytest.mark.skipif(not _OPTIONAL, reason="set GRETL4PY_RUN_OPTIONAL_PACKAGES=1")
def test_extra_combinations_accepts_matrix_and_numpy_view():
    """The same public package call accepts GretlMatrix and NumPy inputs."""
    # Requires the extra package to already be installed locally.
    gretl.include("extra")
    values = gretl.Matrix(np.array([[1.0], [2.0], [3.0], [4.0]]))
    from_matrix = gretl.combinations(values, 1)
    from_numpy = gretl.combinations(values.numpy_view, 1)
    np.testing.assert_allclose(
        np.asarray(from_matrix.numpy_view),
        np.asarray(from_numpy.numpy_view),
    )


@pytest.mark.skipif(not _OPTIONAL, reason="set GRETL4PY_RUN_OPTIONAL_PACKAGES=1")
def test_bma_bundle_return_and_explicit_replication_count():
    """Exercise the BMA bundle return without a million-replication default."""
    # Use an existing installed BMA package/data; do not copy or install it.
    gretl.include("BMA")
    data = gretl.get_data("FLS_41t72.gdt", frompkg="BMA")
    data.new_list(name="Xes", value="2..13")
    opts = gretl.Bundle()
    opts["Nrep"] = 1000    # demo had 10^5 (bitwise XOR, not exponentiation)
    opts["Nrank"] = 5
    opts["model_prior"] = "binomial"
    result = gretl.BMA(
        Y="GDP_growth", X_list="Xes", data=data, Options=opts
    )
    assert result is not None
    assert result["Y"] is not None


@pytest.mark.skipif(not _OPTIONAL, reason="set GRETL4PY_RUN_OPTIONAL_PACKAGES=1")
def test_structism_setup_estimation_and_components():
    """Check a complete installed-package setup/estimate/components cycle."""
    gretl.include("StrucTiSM")
    data = gretl.get_data("longley.csv")
    model = gretl.STSM_setup(data=data, y="gnp", epsilon=True, trend=1, slope=1)
    err = gretl.STSM_estimate(mod=model)
    assert err == 0
    components = gretl.STSM_components(mod=model)
    assert components is not None
