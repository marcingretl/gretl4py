"""Numerical estimator regressions migrated from demo/estimators.py.

Use the *installed* gretl4py package, as enforced by tests/conftest.py.
Each test creates its own dataset and supplies it explicitly, instead of
changing the process-wide default dataset used by the original demonstration.
Reference numbers are from demo/estimators.py (November 2026).
"""

from pathlib import Path

import numpy as np
import pytest

import gretl


_LOCAL_DATA = Path(__file__).parent / "data" / "estimators"


def _data(filename):
    """Load gretl's installed example data, not the source checkout."""
    return gretl.get_data(filename)


def _fixture_data(filename):
    """Load a small, checked-in data fixture originally used in demo/."""
    return gretl.get_data(str(_LOCAL_DATA / filename))


def _close(actual, expected, *, rel=1e-7, abs_=1e-8):
    # Cross-platform LAPACK and nonlinear solvers may differ at the last bits.
    # A failed comparison is diagnostic: do not silently widen the threshold.
    assert actual == pytest.approx(expected, rel=rel, abs=abs_)


def test_ar_selected_lags():
    d = _data("longley.csv")
    # The nested list is equivalent to "1 2 ; employ const prdefl gnp armfrc".
    model = gretl.ar([[1, 2], "employ const prdefl gnp armfrc"], data=d).fit()
    assert model.maxlag == 2


def test_ar1_rho_gls():
    d = _data("longley.csv")
    model = gretl.ar1("employ const prdefl gnp", data=d).fit()
    _close(model.rho_gls, 0.2616616334)


def test_seasonal_arima_initial_missing_residual():
    # Use the bjg.gdt data shipped with gretl4py; demo used bjg.csv.
    d = _data("bjg.gdt")
    model = gretl.arima(depvar="lg", order=(0, 1, 2),
                        seasonal=(0, 1, 1), nc=True, data=d).fit()
    assert np.isnan(np.asarray(model.uhat).flat[0])


def test_biprobit_bic():
    d = _data("greene25_1.gdt")
    # Same two-equation specification as the original example.
    model = gretl.biprobit([[3, 1, 0, 4, 7], [0, 4, 5, 8, 9]], data=d).fit()
    _close(model.bic, 2506.4099430756182)


def test_dynamic_panel_basic_ess():
    d = _data("abdata.gdt")
    model = gretl.dpanel(
        p=2, formula="n w k ; GMM(k,2,3) k GMM(w,2,3)", data=d
    ).fit()
    _close(model.ess, 9.022209618194166)


def test_dynamic_panel_two_step_ess():
    d = _data("abdata.gdt")
    model = gretl.dpanel(
        p=[2, 3], formula="n w k ; GMM(k,2,3) k GMM(w,2,3)",
        data=d, two_step=True
    ).fit()
    _close(model.ess, 7.911977166149402)


@pytest.mark.parametrize("exponential,expected_iters", [(False, 6), (True, 4)])
def test_duration_iterations(exponential, expected_iters):
    d = _fixture_data("kennan.gdt")
    model = gretl.duration("Ti 0 Prod", data=d, exponential=exponential).fit()
    # Iteration counts provide an additional optimizer-path regression check.
    assert model.iters == expected_iters


def test_quantile_regression_bic():
    d = _data("mrw.gdt")
    d.new_series("ly60 = log(gdp60)")
    d.new_series("dlny = log(gdp85) - ly60")
    d.new_series("ngd = 0.05 + (popgrow/100)")
    d.new_series("lngd = log(ngd)")
    d.new_series("linv = log(i_y/100)")
    d.new_series("lschool = log(school/100)")
    d.sample("nonoil", dummy=True)
    model = gretl.quantreg(
        tau=0.75, varlist=[11, 0, 10, 14, 13, 15], data=d
    ).fit()
    _close(model.bic, 80.09721340480137)


def test_garch_bic():
    d = _data("b-g")
    model = gretl.garch("1 1 ; Y", data=d).fit()
    _close(model.bic, 2251.154787743446)


def test_interval_regression_bic():
    d = _data("wtp.gdt")
    # Reproduce the interval limits without sharing a modified dataset.
    d.sample("depvar == 1", restrict=True)
    d.new_series("hi = bidl")
    d.sample("depvar == 2", restrict=True, replace=True)
    d.new_series("lo = bidl")
    d.new_series("hi = bid1")
    d.sample("depvar == 3", restrict=True, replace=True)
    d.new_series("lo = bid1")
    d.new_series("hi = bidh")
    d.sample("depvar == 4", restrict=True, replace=True)
    d.new_series(name="lo", value="bidh")
    d.sample(full=True)
    model = gretl.intreg("lo hi const age female income", data=d).fit()
    _close(model.bic, 811.2237450583831)


def test_heckit_bic():
    d = _data("mroz87.gdt")
    model = gretl.heckit(
        "WW const AX WE CIT ; LFP const WA FAMINC WE", data=d
    ).fit()
    _close(model.bic, 3210.116451720699)


def test_heteroskedastic_regression_bic():
    d = _fixture_data("data4-1.gdt")
    model = gretl.hsk("price ~ 0 + sqft + bedrms", data=d).fit()
    _close(model.bic, 51.979783893886214)


def test_lad_bic():
    d = _data("longley.csv")
    gretl.set("seed", 1234)
    model = gretl.lad("employ ~ const + prdefl + gnp", data=d).fit()
    _close(model.bic, 259.34905425955566)


def test_logistic_bic():
    d = _data("longley.csv")
    gretl.set("seed", 1234)
    model = gretl.logistic("employ ~ const + prdefl + gnp", data=d).fit()
    _close(model.bic, -39.699260872353044)


def test_tobit_bic():
    d = _data("tobit.gdt")
    model = gretl.tobit(
        [5, 0, 2, 3, 8, 7], llimit=400, rlimit=600.7, data=d
    ).fit()
    _close(model.bic, 859.3742496817283)


def test_tsls_ess():
    d = _data("penngrow.gdt")
    model = gretl.tsls("Y Y(-1) X ; 0 X Y(-2)", data=d).fit()
    _close(model.ess, 8.019127932937186)


def test_var_log_determinant():
    d = _data("denmark.gdt")
    model = gretl.var(
        order=2, ylist="LRM LRY", xlist="IBO IDE",
        data=d, trend=True, seasonals=True
    ).fit()
    _close(model.ldet, -16.441467597306143)


def test_vecm_log_likelihood():
    d = _data("denmark.gdt")
    model = gretl.vecm(
        order=2, rank=1, ylist="LRM LRY", xlist="IBO IDE",
        data=d, rc=True, seasonals=True
    ).fit()
    _close(model.lnl, 277.6983198360378)


def test_wls_bic():
    d = _data("longley.csv")
    model = gretl.wls("year employ const prdefl gnp", data=d).fit()
    _close(model.bic, 379.860968614329)
