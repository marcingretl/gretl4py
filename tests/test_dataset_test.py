"""
GretlDataset.test() regression tests - the corrgm/fractint/normtest/runs
missing-bounds-check bug (round-2/round-3 finding), reported and reproduced
by Marcin against a real time-series dataset.

Background: an empty-but-valid gretl list (e.g. `list L = null`) makes
gretllist() return a genuinely zero-length list (err_ == 0, not an error).
The corrgm/fractint/normtest/runs branches read element [1] of that list
unconditionally - an out-of-bounds read whose garbage value happened to
decode as variable 0 ("const") in testing, but is not guaranteed to: a
different heap layout could just as easily produce a real variable's index
with no error raised at all (this is exactly what happened for `runs`
before the fix - it silently ran against the wrong variable and returned
normally).
"""
import pytest


TESTS_TAKING_Y = {"corrgm": {"y": "L", "order": 2}, "fractint": {"varlist": "L", "order": 2}}
TESTS_TAKING_VARLIST = {"normtest": {"varlist": "L"}, "runs": {"varlist": "L"}}


@pytest.mark.parametrize("test_type,kwargs", list(TESTS_TAKING_Y.items()) + list(TESTS_TAKING_VARLIST.items()))
def test_empty_list_rejected(bjg_dataset, test_type, kwargs):
    bjg_dataset.new_list("L = null")
    with pytest.raises(ValueError):
        bjg_dataset.test(test_type, **kwargs)


def test_corrgm_normal_call_still_works(bjg_dataset):
    bjg_dataset.test("corrgm", y="g", order=5)
    assert bjg_dataset.tests[-1]["pvalue"] is not None


def test_fractint_normal_call_still_works(bjg_dataset):
    bjg_dataset.test("fractint", varlist="g", order=5)
    assert bjg_dataset.tests[-1]["pvalue"] is not None


def test_normtest_normal_call_still_works(bjg_dataset):
    bjg_dataset.test("normtest", varlist="g")
    assert bjg_dataset.tests[-1]["pvalue"] is not None


def test_runs_normal_call_still_works(bjg_dataset):
    bjg_dataset.test("runs", varlist="g")
    assert bjg_dataset.tests[-1] is not None


def test_multi_variable_list_rejected_not_silently_truncated(bjg_dataset):
    # before the fix, passing 2 variables silently used only the first one,
    # with no warning at all - now it's a clean rejection instead
    with pytest.raises(ValueError):
        bjg_dataset.test("corrgm", y="g lg", order=5)


def test_vartest_sibling_unaffected(bjg_dataset):
    # vartest already had the correct (!= 2) check before this fix; make
    # sure touching its siblings didn't disturb it
    with pytest.raises(ValueError):
        bjg_dataset.test("vartest", varlist="g")  # needs exactly two series


# --- pedantic sweep: every OTHER branch in the same dispatch table ---------
#
# Only corrgm/fractint/normtest/runs/vartest index directly into
# ex_var.get()[N] in gretl4py's own code - that's the bug class that was
# fixed. The other branches (adf, bds, coint, difftest, johansen, kpss,
# levinlin, meantest) just pass the whole list pointer through to
# libgretl's own C functions, so there's no equivalent gretl4py-side OOB
# read to fix there. This sweep checks the weaker but still meaningful
# property for those - "an empty list doesn't crash the process" - without
# claiming to have verified libgretl's own internal handling is correct.
# Some of these (adf/kpss/meantest, as of this writing) return normally
# rather than raising - that's *not* asserted as correct here, only as
# "doesn't crash"; if that ever needs stronger scrutiny, it's a libgretl
# question, not a gretl4py one.

OTHER_DISPATCH_TYPES = ["adf", "bds", "coint", "difftest", "fractint",
                        "johansen", "kpss", "levinlin", "meantest"]


@pytest.mark.parametrize("test_type", OTHER_DISPATCH_TYPES)
def test_other_dispatch_branches_survive_empty_list(bjg_dataset, test_type):
    bjg_dataset.new_list("L = null")
    try:
        bjg_dataset.test(test_type, varlist="L", order=1)
    except Exception:
        pass  # any clean exception is fine - the point is no crash
