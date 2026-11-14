"""
GretlPackage regression tests - covers the transfer_ownership() bug
(GretlPackageRes never took ownership of the value gretl_function_exec()
returned, so every call into a contributed package leaked its return value,
regardless of type: matrix, string, or bundle).
"""
import resource

import pytest

import gretl


def _rss_kb():
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss


def test_package_loads_and_lists_functions(criteria_package):
    assert "criteria" in criteria_package.get_functions()


def test_package_function_call_correctness(criteria_package):
    result = criteria_package._function_exec("criteria", ess=100.0, n=50, k=3, quiet=1)
    assert result.rows == 1 and result.cols == 4  # AIC, BIC, HQC, AICc


def test_package_function_return_value_no_leak(criteria_package):
    before = _rss_kb()
    for _ in range(20_000):
        criteria_package._function_exec("criteria", ess=100.0, n=50, k=3, quiet=1)
    after = _rss_kb()
    assert after - before < 20_000, f"RSS grew by {after - before} KB across 20k package calls"


# --- error paths -------------------------------------------------------------
#
# Not known-bug regressions - pedantic coverage that misuse fails cleanly
# (a Python exception) rather than crashing, since these all eventually
# reach the same unpack_kwargs_package_()/gretl_function_exec() machinery
# the leak fix lives in.

def test_unknown_function_name_raises_cleanly(criteria_package):
    with pytest.raises(Exception):
        criteria_package._function_exec("this_function_does_not_exist")


def test_missing_required_argument_raises_cleanly(criteria_package):
    with pytest.raises(Exception):
        criteria_package._function_exec("criteria", n=50, k=3)  # missing 'ess'


def test_wrong_argument_type_raises_cleanly(criteria_package):
    with pytest.raises(Exception):
        criteria_package._function_exec("criteria", ess="not a number", n=50, k=3)


def test_unknown_package_name_raises_cleanly():
    with pytest.raises(Exception):
        gretl.include("this_package_does_not_exist_either")
