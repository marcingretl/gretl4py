"""
 *
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 """
import inspect
import sys
import os
import webbrowser
import numpy as np
import pandas as pd
from typing import TypedDict, NotRequired, Dict, List, Any
from typing import Optional
from .gretl4py_addons import _pd_to_pandas_str_format
from ._plotting import _plotfitted, _plotresiduals
from . import _gretl

# #######################################################
#   GretlModel class
# #######################################################
def _attrs(self: "GretlModel"):
    for attname in dir(self):
        if not attname.startswith("_"):
            attr = getattr(self, attname)
            if isinstance(attr, float):
                print(f'{attname}: {attr:.8g}')
            elif isinstance(attr, int):
                print(f'{attname}: {attr}')
            elif isinstance(attr, str) and len(attr) < 32:
                print(f'{attname}: {attr}')
            else:
                print(f'{attname}: {type(attr)}')

def _oddsratio(self: "Model_logit"):
    print("WARNING: oddsratios() is deprecated; use model.oddsratios instead\n")

    return self.oddsratios

def _model_plots(
    self: "GretlModel",
    kind: str,
    eqn: int = 0,
    multiple: bool = False,
    against: str | None = None,
    backend: str | None = None
):
    """
    General dispatcher for model-based plots.

    This method provides a unified entry point for generating plots associated
    with a fitted model, such as fitted-versus-actual plots or residual plots.
    The specific plot to generate is selected via the ``kind`` argument and
    delegated to the corresponding internal plotting routine.

    Parameters
    ----------
    kind : str
        Type of plot to generate. Supported values are:

        - ``"fitted"`` : Plot fitted values against observed data.
        - ``"residuals"`` : Plot regression residuals.

    eqn : int, optional
        Equation number to plot. Use ``0`` to plot all equations or dependent
        variables (default).

    multiple : bool, optional
        If ``False`` (default), a separate figure is produced for each variable.
        If ``True``, all plots are arranged as subplots in a single figure.

    against : str, optional
        Name of a variable in the dataset to plot against instead of the default
        time or observation index. If provided, data are sorted by this variable
        before plotting.

    backend : str, optional
        Name of the matplotlib backend to use (e.g. ``"TkAgg"``, ``"QtAgg"``,
        ``"Agg"``). If None, the default backend selection logic is used.

    Raises
    ------
    ValueError
        If ``kind`` does not correspond to a supported plot type.

    Notes
    -----
    - This method is intended as an internal wrapper for all model-related plots.
    - Backend selection applies to all plots generated during this call.
    - Public plotting methods may delegate to this dispatcher to ensure
      consistent behavior.

    Examples
    --------
    >>> model.plot("fitted")
    >>> model.plot("residuals", multiple=True)
    >>> model.plot("fitted", against="lg", backend="QtAgg")
    """
    if self.estimated is False:
        raise RuntimeError("Model was not estimated yet")

    if kind == "fitted":
        _plotfitted(self, eqn, multiple, against, backend)
    elif kind == "residuals":
        _plotresiduals(self, eqn, multiple, against, backend)
    else:
        raise ValueError(f"unknown plotting type: {kind}")

def _add(self: "Model", varlist: str, **kwargs) -> "Bundle":
    """
    Test or select variables for addition to the model.

    By default, estimates an augmented model including the specified
    variables and performs a joint Wald test of their significance.
    Alternatively, an LM test or forward stepwise selection can be used.

    Parameters
    ----------
    varlist : str | list
        Variables to add to the model, or candidate variables when
        ``auto`` is specified.

    Other Parameters
    ----------------
    lm : bool
        Perform an LM test instead of estimating an augmented model.
        Available for OLS models only.
    auto : str | float
        Perform forward stepwise selection using ``"AIC"``, ``"BIC"``,
        ``"HQC"``, or a significance level as the selection criterion.
        Available for OLS models only.
    quiet : bool
        Print only the basic test result.
    silent : bool
        Suppress printed output.
    vcv : bool
        Print the covariance matrix for the augmented model.
    both : bool
        For IV models, add the variables to both the regressors and
        instruments.

    Notes
    -----
    By default, the augmented model replaces the original model as the
    current model. With ``lm=True``, the original model is retained.
    The ``lm`` and ``auto`` options cannot be combined.
    """
    return self.test(type="add", varlist=varlist, **kwargs)
_add.__name__ = "add"
_add.__qualname__ = "Model.add"

def _arch(self: "Model", **kwargs) -> "Bundle":
    return self.test(type="arch", **kwargs)
_arch.__name__ = "arch"
_arch.__qualname__ = "Model.arch"

def _autocorr(self: "Model", order: int, **kwargs) -> "Bundle":
    return self.test(type="autocorr", order=order, **kwargs)
_autocorr.__name__ = "autocorr"
_autocorr.__qualname__ = "Model.autocorr"

def _bkw(self: "Model", **kwargs) -> "Bundle":
    """
    Compute Belsley-Kuh-Welsch collinearity diagnostics.

    Calculates the BKW table for diagnosing the degree and sources of
    collinearity among the regressors, based on eigenanalysis of the
    regressor correlation structure.

    Other Parameters
    ----------------
    quiet : bool
        Suppress printed output.

    Notes
    -----
    The model must contain at least two independent variables. The BKW
    diagnostics follow Belsley, Kuh and Welsch (1980) and provide a more
    detailed analysis of collinearity than variance inflation factors.
    """
    return self.test(type="bkw", **kwargs)
_bkw.__name__ = "bkw"
_bkw.__qualname__ = "Model.bkw"

def _chow(self: "Model", target: str, **kwargs) -> "Bundle":
    """
    Perform the Chow test for structural stability.

    Tests for a structural break at a specified observation, or for a
    difference between groups defined by a dummy variable.

    Parameters
    ----------
    target : str
        Break observation, or the name of a dummy variable when
        ``dummy=True``.

    Other Parameters
    ----------------
    dummy : bool
        Interpret ``target`` as the name of a dummy variable rather than
        a break observation.
    quiet : bool
        Suppress estimates for the augmented model.
    limit_to : str | list
        Restrict the test to a specified subset of regressors.

    Notes
    -----
    Without ``dummy=True``, ``target`` identifies the observation at which
    the sample is split for the structural-break test.
    """
    return self.test(type="chow", obs=target, **kwargs)
_chow.__name__ = "chow"
_chow.__qualname__ = "Model.chow"

def _cusum(self: "Model", **kwargs) -> "Bundle":
    return self.test(type="cusum", **kwargs)
_cusum.__name__ = "cusum"
_cusum.__qualname__ = "Model.cusum"

def _leverage(self: "Model", **kwargs) -> "Bundle":
    return self.test(type="leverage", **kwargs)
_leverage.__name__ = "leverage"
_leverage.__qualname__ = "Model.leverage"

def _nonlinearity(self: "Model", mode: str, **kwargs) -> "Bundle":
    if mode not in ["logs", "squares"]:
        raise ValueError(f'Test for nonlinearity, unknown mode: {mode}')
    return self.test(type=mode, **kwargs)
_nonlinearity.__name__ = "nonlinearity"
_nonlinearity.__qualname__ = "Model.nonlinearity"

def _normality(self: "Model", **kwargs) -> "Bundle":
    return self.test(type="normality", **kwargs)
_normality.__name__ = "normality"
_normality.__qualname__ = "Model.normality"

def _omit(self: "Model", varlist: str | list, **kwargs) -> "Bundle":
    return self.test(type="omit", varlist=varlist, **kwargs)
_omit.__name__ = "omit"
_omit.__qualname__ = "Model.omit"

def _qlrtest(self: "Model", **kwargs) -> "Bundle":
    return self.test(type="qlrtest", **kwargs)
_qlrtest.__name__ = "qlrtest"
_qlrtest.__qualname__ = "Model.qlrtest"

def _reset(self: "Model", **kwargs) -> "Bundle":
    return self.test(type="reset", **kwargs)
_reset.__name__ = "reset"
_reset.__qualname__ = "Model.reset"

def _white(self: "Model", white_nocross: bool = False, **kwargs) -> "Bundle":
    if white_nocross:
        return self.test(type="white-nocross", **kwargs)
    else:
        return self.test(type="white", **kwargs)
_white.__name__ = "white"
_white.__qualname__ = "Model.white"

def _vif(self: "Model", **kwargs) -> "Bundle":
    return self.test(type="vif", **kwargs)
_vif.__name__ = "vif"
_vif.__qualname__ = "Model.vif"

# #######################################################
#   GretlModel_nl class
# #######################################################
class NLSpec(TypedDict, total=False):
    formula: str
    params: str
    param_names: str
    initvals: Dict[str, str]
    statements: List[str]
    deriv: Dict[str, str]
    orthog: List[str]
    weights: str

def mle(formula: str | NLSpec, **options: Any) -> "Model_mle":
    """
    MLE factory for gretl. Accepts either:

    - a string formula
    - an NLSpec dictionary

    Extra keyword arguments in **options are forwarded to _gretl._mle().
    """

    # via NLSpec
    if isinstance(formula, dict):
        loglik = formula.get("formula", "")
        params = formula.get("params", "")
        param_names = formula.get("param_names", "")
        initvals = formula.get("initvals", {})
        deriv = formula.get("deriv", {})
        statements = formula.get("statements", [])

        return _gretl._mle(loglik, params=params,
                            param_names=param_names,
                            initvals=initvals,
                            statements=statements,
                            deriv=deriv, **options)

    # passed as string + kwargs
    if isinstance(formula, str):
        return _gretl._mle(loglik=formula, **options)

    # if error
    raise TypeError(f"Unsupported MLE specification of type {type(formula).__name__}; "
        "expected a string formula or an NLSpec dictionary.")

def nls(formula: str | NLSpec, **options: Any) -> "Model_nls":
    """
    NLS factory for gretl. Accepts either:

    - a string formula
    - an NLSpec dictionary

    Extra keyword arguments in **options are forwarded to _gretl._mle().
    """

    # via NLSpec
    if isinstance(formula, dict):
        function = formula.get("formula", "")
        params = formula.get("params", "")
        param_names = formula.get("param_names", "")
        initvals = formula.get("initvals", {})
        deriv = formula.get("deriv", {})

        return _gretl._nls(function, params=params,
                            param_names=param_names,
                            initvals=initvals,
                            deriv=deriv, **options)

    # passed as string + kwargs
    if isinstance(formula, str):
        return _gretl._nls(function=formula, **options)

    # if error
    raise TypeError(f"Unsupported NLS specification of type {type(formula).__name__}; "
        "expected a string formula or an NLSpec dictionary.")

def gmm(formula: str | NLSpec, **options: Any) -> "Model_gmm":
    """
    GMM factory for gretl. Accepts either:

    - a string formula
    - an NLSpec dictionary

    Extra keyword arguments in **options are forwarded to _gretl._mle().
    """

    # via NLSpec
    if isinstance(formula, dict):
        resid = formula.get("formula", "")
        params = formula.get("params", "")
        param_names = formula.get("param_names", "")
        initvals = formula.get("initvals", {})
        statements = formula.get("statements", [])
        orthog = formula.get("orthog", [])
        weights = formula.get("weights", "")

        return _gretl._gmm(resid, params=params,
                            param_names=param_names,
                            initvals=initvals,
                            statements=statements,
                            orthog=orthog, weights=weights,
                            **options)

    # passed as string + kwargs
    if isinstance(formula, str):
        return _gretl._gmm(resid=formula, **options)

    # if error
    raise TypeError(f"Unsupported NLS specification of type {type(formula).__name__}; "
        "expected a string formula or an NLSpec dictionary.")

# #######################################################
#   GretlModel_NME class
# #######################################################
class MESpec(TypedDict, total=False):
    equations: List[str]
    instruments: List[str]
    identities: List[str]
    endogs: str

# #######################################################
#   Dataset class
# #######################################################
def _matrix_functions_wrapper(func_name: str):
    """
    Returns a method that wraps a _gretl matrix-based function.

    The method has signature:
        method(self, d: str, series: str, param: Optional[Any] = None) -> np.ndarray
    """
    def method(self, d: str, series: str, param: Optional[any] = None) -> np.ndarray:
        func_call = getattr(_gretl, func_name)
        # call the _gretl function with dataset, series, optional param
        return func_call(d, self.get_series(series), param)

    return method

def get_data(src: str | dict | int, **kwargs) -> "Dataset":
    """Load dataset from file path or dictionary.

    Parameters:
    src (str | dict | int): Either a file path or a dictionary or dataset's id.

    Returns:
    GretlDataset: A dataset object.
    """
    if isinstance(src, str):
        source = src  # file path case
    elif isinstance(src, int):
        if kwargs:
            raise ValueError("You cannot use kwargs when attaching already registered dataset")
        source = str(src)
    elif isinstance(src, dict):
        if kwargs:
            raise ValueError("You cannot use kwargs when loading data from dict")
        source = None
        frame = inspect.currentframe()
        try:
            outer_frame = inspect.getouterframes(frame)[1]
            caller_frame = outer_frame[0]  # extract caller's frame
            string = inspect.getframeinfo(caller_frame).code_context[0].strip()

            # extract variable name from the function call
            args = string[string.find('(') + 1:-1].split(',')
            if len(args) == 1:
                source = args[0].split('=')[1].strip() if '=' in args[0] else args[0].strip()
        except Exception:
            pass  # if direct extraction fails, scan variables
        finally:
            del frame  # cleanup to prevent reference cycles

        # try to find the variable in caller's scope if name extraction failed
        if not source:
            caller_globals, caller_locals = caller_frame.f_globals, caller_frame.f_locals
            source = next((var for var, val in {**caller_globals, **caller_locals}.items() if val is src), "unknown")

        source += " (dict)"
    else:
        raise RuntimeError(f'Unsupported data src type: {type(src).__name__}\n')

    # create dataset
    return _gretl._get_data(src, source=source, **kwargs)

def show_data_status():
    """List of available datasets."""

    # get ids of all registered datasets
    reg_datasets = sorted(_gretl._get_datasets_ids())
    if len(reg_datasets) > 0:
        print('All currently registered datasets (id): ', reg_datasets, '\n')

    # get the caller's global and local variables
    caller_globals = sys._getframe(1).f_globals
    caller_locals = sys._getframe(1).f_locals

    # combine both to search for datasets
    all_globals = {**caller_globals, **caller_locals}

    datasets_found = [name for name, value in all_globals.items() if isinstance(value, _gretl.Dataset)]

    if len(datasets_found) == 0:
        print('No datasets were loaded.')
    else:
        print('Datasets available to user(id, source):')
        for d in datasets_found:
            dset = all_globals[d]
            if dset.is_default is True:
                default = "(default)"
            else:
                default = ""
            print(f'{d}: {dset.id:d}, {dset.source:s} {default:s} - referenced by models: {list(dset.linked_models_list)}')

def _dataset_to_pandas(self: "Dataset") -> pd.DataFrame:
    """Method for exporting _gretl.Dataset instance as Pandas DataFrame"""

    # we grab data as dict + some characteristics
    as_d = self.to_dict()
    datatype = self.get_accessor("$datatype")
    pdcode = self.get_accessor("$pd")

    # base frame
    df = pd.DataFrame(
        data = as_d["data_array"],
        columns = as_d["varname"],
        index = as_d["obs_labels"])

    # we deal with dataset types
    if datatype == 1:
        # cross-sectional
        return df
    elif datatype == 2:
        # time-series
        freq = _pd_to_pandas_str_format(pdcode)

        if freq is not None:
            try:
                df.index = pd.PeriodIndex(df.index, freq=freq)
            except Exception:
                pass
        return df
    elif datatype == 3:
        # panel
        panel_unit = np.array(self.get_accessor("$unit"))
        panel_freq = self.get_accessor("$panelpd")

        # generate sequential time values per unit
        panel_time = np.zeros_like(panel_unit)
        for u in np.unique(panel_unit):
            mask = panel_unit == u
            panel_time[mask] = np.arange(mask.sum())

        # base multi-index
        df.index = pd.MultiIndex.from_arrays([panel_unit, panel_time],
                                                names=["unit", "time"])

        # if we have real periodicity - convert time level _only_
        freq = _pd_to_pandas_str_format(panel_freq)

        if freq is not None:
            try:
                df = df.set_index(df.index.set_levels(
                                    pd.PeriodIndex(df.index.levels[1],
                                                    freq=freq),
                                                    level="time"))
            except Exception:
                pass
        return df

    # unknown datatype -> return raw df
    return df

def _to_dataframe(self: "Dataset") -> pd.DataFrame:
    print("*** This method is deprecated and will be removed in v0.7. "
        "Please use `to_pandas()` instead. ***")
    return _dataset_to_pandas(self)

def _package_get_help(self: "Package"):
    """
    Display package help.

    - Opens PDF documentation if available
    - Otherwise prints text help
    """
    info = self._get_help()

    if not info or len(info) != 2:
        raise RuntimeError("Invalid help information returned from backend")

    kind, payload = info

    if kind == "pdf":
        pdf_path = os.path.abspath(payload)

        if not os.path.exists(pdf_path):
            raise FileNotFoundError(pdf_path)

        # OS-independent: uses default PDF viewer
        webbrowser.open(f"file://{pdf_path}")
    elif kind == "txt":
        print(payload)
    else:
        raise RuntimeError(f"Unknown help type: {kind}")

def _dataset_old_test(self: "Dataset", type: str, **kwargs) -> "Bundle":
    print(f'WARNING: Dataset::test(type="{type}") method is deprecated; use Dataset::{type}() instead\n')

    return self._test(type=type, **kwargs)

def _adf(self: "Dataset", order: int, varlist: str | list, **kwargs) -> "Bundle":
    """Perform Augmented Dickey-Fuller unit-root tests.

    Parameters
    ----------
    order : int
        Lag order; use -1 for the Schwert rule.
    varlist : str | list
        Variable(s) to test.

    Keyword options
    ---------------
    nc, c, ct, ctt : bool
        Select deterministic terms.
    seasonals : bool
        Include seasonal dummies.
    gls : bool
        Use GLS demeaning or detrending.
    verbose : bool | str
        Print additional information; use "max" for full output.
    quiet : bool
        Suppress output.
    difference : bool
        Test the first difference.
    test_down : bool | str
        Select lag order automatically (AIC, BIC, or "tstat").
    perron_qu : bool
        Use the Perron-Qu lag-selection refinement.
    """
    return self._test(type="adf", order=order, varlist=varlist, **kwargs)
_adf.__name__ = "adf"
_adf.__qualname__ = "Dataset.adf"

def _bds(self: "Dataset", order: int, x: str | list, **kwargs) -> "Bundle":
    """
    Perform the BDS test for nonlinearity/IID violations.

    The BDS test detects general departures from linearity or IID behavior in a
    series, and is commonly applied to regression residuals. The `order` argument
    sets the maximum embedding dimension and must be at least 2.

    Parameters
    ----------
    order : int
        Maximum dimension used in the test.
    x : str
        Name of the series to test.
    corr1 : float, optional
        Target first-order correlation integral for selecting epsilon.
    sdcrit : float, optional
        Set epsilon as a multiple of the standard deviation of `x`.
    boot : int, optional
        Number of bootstrap replications. Use 0 to disable bootstrapping.
    quiet : bool, optional
        Suppress printed output.

    Returns
    -------
    Matrix
        A 2 x (`order` - 1) matrix containing test statistics in the first row
        and p-values in the second row.
    """
    return self._test(type="bds", order=order, varlist=x, **kwargs)
_bds.__name__ = "bds"
_bds.__qualname__ = "Dataset.bds"

def _coint(self: "Dataset", order: int, depvar: str, indepvars: str | list, **kwargs) -> "Bundle":
    """
    Perform the Engle-Granger cointegration test.

    Tests for cointegration between a dependent variable and one or more
    independent variables. By default, Dickey-Fuller tests are first applied
    to the individual variables, followed by estimation of the cointegrating
    regression and a DF test on its residuals.

    Parameters
    ----------
    order : int
        Lag order for the Dickey-Fuller tests. With ``test_down=True``,
        specifies the maximum lag order.
    depvar : str
        Dependent variable in the cointegrating regression.
    indepvars : str | list
        Independent variables in the cointegrating regression.

    Other Parameters
    ----------------
    nc : bool
        Do not include a constant.
    ct : bool
        Include a constant and linear trend.
    ctt : bool
        Include a constant and quadratic trend.
    seasonals : bool
        Include seasonal dummy variables.
    skip_df : bool
        Skip DF tests on the individual variables.
    test_down : bool | str
        Select the lag order automatically by testing down, optionally
        specifying the selection criterion.
    verbose : bool
        Print additional regression details.
    silent : bool
        Suppress output.

    Notes
    -----
    The ``nc``, ``ct``, and ``ctt`` options are mutually exclusive.
    P-values are based on MacKinnon (1996).
    """
    return self._test(type="coint", y=depvar, varlist=indepvars, **kwargs)
_coint.__name__ = "coint"
_coint.__qualname__ = "Dataset.coint"

def _corrgm(self: "Dataset", y: str, order: int, **kwargs) -> "Bundle":
    """
    Compute the correlogram of a time series.

    Calculates the autocorrelation function (ACF) and, by default, the
    partial autocorrelation function (PACF) using the Durbin-Levinson
    algorithm. The Ljung-Box Q statistic for testing the white-noise
    hypothesis is also computed.

    Parameters
    ----------
    y : str
        Series for which the correlogram is computed.
    order : int, optional
        Maximum lag. If omitted, the lag order is selected automatically
        based on the data frequency and sample size.

    Other Parameters
    ----------------
    plot : str
        Plot mode (``"none"`` or ``"display"``) or output filename.
    quiet : bool
        Suppress printed output.
    acf_only : bool
        Compute only the ACF, omitting partial autocorrelations.
    bartlett : bool
        Use Bartlett standard errors for the ACF.

    Notes
    -----
    By default, ACF standard errors are computed under the white-noise
    null hypothesis. Bartlett standard errors instead provide bounds
    associated with an MA(k) process.
    """
    return self._test(type="corrgm", y=y, order=order, **kwargs)
_corrgm.__name__ = "corrgm"
_corrgm.__qualname__ = "Dataset.corrgm"

def _difftest(self: "Dataset", x: str, y: str, **kwargs) -> "Bundle":
    """
    Perform a nonparametric test for a difference between two samples.

    Supports the Sign test, Wilcoxon rank-sum test, and Wilcoxon
    signed-rank test. The samples may be given as two series, or, for the
    rank-sum test, obtained by splitting one series according to a dummy
    variable.

    Parameters
    ----------
    x : str
        First sample.
    y : str, optional
        Second sample. Required unless ``split_by`` is specified.

    Other Parameters
    ----------------
    split_by : str
        Dummy variable defining two samples from ``x``. Available only
        with the rank-sum test.
    sign : bool
        Perform the Sign test. This is the default.
    rank_sum : bool
        Perform the Wilcoxon rank-sum test for independent samples.
    signed_rank : bool
        Perform the Wilcoxon signed-rank test for paired samples.
    verbose : bool
        Print ranking details for the Wilcoxon tests.
    quiet : bool
        Suppress printed output.

    Notes
    -----
    The Sign and signed-rank tests require paired observations. The
    rank-sum test permits samples of different sizes and can be used with
    ``split_by``.
    """
    return self._test(type="difftest", varlist=x + " " + y, **kwargs)
_difftest.__name__ = "difftest"
_difftest.__qualname__ = "Dataset.difftest"

def _fractint(self: "Dataset", series: str, **kwargs) -> "Bundle":
    """
    Test a time series for fractional integration (long memory).

    Tests the null hypothesis that the fractional integration order is
    zero. By default, the Local Whittle estimator is used; the GPH test
    can be selected as an alternative or both tests can be performed.

    Parameters
    ----------
    series : str
        Series to test.
    order : int, optional
        Order used for estimation. If omitted, it is selected automatically
        as the lesser of T/2 and T**0.6.

    Other Parameters
    ----------------
    gph : bool
        Use the Geweke-Porter-Hudak (GPH) test instead of Local Whittle.
    all : bool
        Perform both the Local Whittle and GPH tests.
    quiet : bool
        Suppress printed output.

    Notes
    -----
    The Local Whittle estimator follows Robinson (1995), while the GPH
    test follows Geweke and Porter-Hudak (1983).
    """
    return self._test(type="fractint", varlist=series, **kwargs)
_fractint.__name__ = "fractint"
_fractint.__qualname__ = "Dataset.fractint"

def _johansen(self: "Dataset", order: int, varlist: str | list, **kwargs) -> "Bundle":
    """
    Perform the Johansen cointegration test.

    Tests for the cointegration rank among a set of endogenous variables
    using the Johansen trace and maximum-eigenvalue tests.

    Parameters
    ----------
    order : int
        Lag order of the VAR system.
    ylist : str | list
        Endogenous variables to include in the cointegration system.

    Other Parameters
    ----------------
    xlist : str | list
        Exogenous variables entering the system unrestrictedly.
    rxlist : str | list
        Exogenous variables restricted to the cointegration space.
    nc : bool
        Include no constant.
    rc : bool
        Include a restricted constant.
    uc : bool
        Include an unrestricted constant. This is the default.
    crt : bool
        Include a constant and restricted trend.
    ct : bool
        Include a constant and unrestricted trend.
    seasonals : bool
        Include centered seasonal dummy variables.
    asy : bool
        Use asymptotic rather than sample-size-adjusted p-values.
    quiet : bool
        Print only the test results.
    silent : bool
        Suppress printed output.
    verbose : bool
        Print details of auxiliary regressions.

    Notes
    -----
    The deterministic-term options ``nc``, ``rc``, ``uc``, ``crt``, and
    ``ct`` are mutually exclusive. P-values are computed using Doornik's
    gamma approximation.
    """
    return self._test(type="johansen", varlist=varlist, order=order, **kwargs)
_johansen.__name__ = "johansen"
_johansen.__qualname__ = "Dataset.johansen"

def _kpss(self: "Dataset", order: int, varlist: str | list, **kwargs) -> "Bundle":
    """
    Perform the KPSS test for stationarity.

    Tests the null hypothesis that each specified series is stationary
    around a constant, or around a deterministic linear trend when
    ``trend=True``.

    Parameters
    ----------
    order : int
        Window size for Bartlett smoothing. A negative value selects the
        window size automatically based on the sample size.
    varlist : str | list
        Series to test.

    Other Parameters
    ----------------
    trend : bool
        Test for stationarity around a deterministic linear trend.
    seasonals : bool
        Include seasonal dummy variables.
    verbose : bool
        Print results of the auxiliary regression.
    quiet : bool
        Suppress printed output.
    difference : bool
        Apply the test to the first difference of each series.

    Notes
    -----
    For panel data, only one variable can be tested at a time and the
    overall stationarity test is based, where possible, on Choi's method.
    Critical values are based on the response-surface estimates of
    Sephton (1995).
    """
    return self._test(type="kpss", varlist=varlist, order=order, **kwargs)
_kpss.__name__ = "kpss"
_kpss.__qualname__ = "Dataset.kpss"

def _levinlin(self: "Dataset", order: int | list, series: str, **kwargs) -> "Bundle":
    """
    Perform the Levin-Lin-Chu panel unit-root test.

    Tests the null hypothesis that all individual series in the panel have
    a unit root against the alternative that none has a unit root. A common
    AR(1) coefficient across panel units is assumed.

    Parameters
    ----------
    order : int | array-like
        Lag order for the ADF regressions. A scalar applies the same order
        to all panel units; an array-like value may specify a separate
        order for each unit.
    series : str
        Panel series to test.

    Other Parameters
    ----------------
    nc : bool
        Run the test without a constant.
    ct : bool
        Include a constant and linear trend.
    quiet : bool
        Suppress printed output.
    verbose : bool
        Print results for individual panel units.

    Notes
    -----
    By default, the auxiliary ADF regressions include a constant. The
    ``nc`` and ``ct`` options are mutually exclusive.
    """
    return self._test(type="levinlin", order=order, varlist=series, **kwargs)
_levinlin.__name__ = "levinlin"
_levinlin.__qualname__ = "Dataset.levinlin"

def _meantest(self: "Dataset", x: str, y: str, **kwargs) -> "Bundle":
    """
    Test for equality of means between two samples.

    Performs a t-test of the null hypothesis that the population means are
    equal. The samples may be given as two series or obtained by splitting
    one series according to a dummy variable.

    Parameters
    ----------
    x : str
        First sample.
    y : str, optional
        Second sample. Required unless ``split_by`` is specified.

    Other Parameters
    ----------------
    split_by : str
        Dummy variable defining two samples from ``x``.
    unequal_vars : bool
        Allow unequal variances, using the Satterthwaite approximation
        for the degrees of freedom.
    paired : bool
        Perform a paired test of a zero mean difference.
    quiet : bool
        Suppress printed output.
    robust : bool
        Use a test robust to serial correlation for time-series data.

    Notes
    -----
    By default, the two samples are assumed to have equal variances.
    ``split_by`` cannot be combined with ``paired`` or ``robust``.
    """
    return self._test(type="meantest", varlist=x + " " + y, **kwargs)
_meantest.__name__ = "meantest"
_meantest.__qualname__ = "Dataset.meantest"

def _normtest(self: "Dataset", series: str, **kwargs) -> "Bundle":
    """
    Test a series for normality.

    Performs a normality test using the Doornik-Hansen, Shapiro-Wilk,
    Lilliefors, or Jarque-Bera procedure. By default, the Doornik-Hansen
    test is used.

    Parameters
    ----------
    series : str
        Series to test for normality.

    Other Parameters
    ----------------
    dhansen : bool
        Perform the Doornik-Hansen test. This is the default.
    swilk : bool
        Perform the Shapiro-Wilk test.
    lillie : bool
        Perform the Lilliefors test.
    jbera : bool
        Perform the Jarque-Bera test.
    all : bool
        Perform all available normality tests.
    quiet : bool
        Suppress printed output.

    Notes
    -----
    The Doornik-Hansen and Shapiro-Wilk tests are generally preferred for
    their small-sample properties. With ``all=True``, the recorded test
    statistic and p-value are those from the Doornik-Hansen test.
    """
    return self._test(type="normtest", varlist=series, **kwargs)
_normtest.__name__ = "normtest"
_normtest.__qualname__ = "Dataset.normtest"

def _runs(self: "Dataset", series: str, **kwargs) -> "Bundle":
    """
    Perform the nonparametric runs test for randomness.

    Tests the randomness of a series based on runs of consecutive positive
    or negative values.

    Parameters
    ----------
    series : str
        Series to test.

    Other Parameters
    ----------------
    difference : bool
        Test the first difference of the series, so that runs represent
        consecutive increases or decreases.
    equal : bool
        Assume that positive and negative values are equiprobable.

    Notes
    -----
    By default, the test focuses on independence without assuming equal
    probabilities of positive and negative values. To test randomness
    around the median, subtract the series median before applying the test.
    """
    return self._test(type="runs", varlist=series, **kwargs)
_runs.__name__ = "runs"
_runs.__qualname__ = "Dataset.runs"

def _vartest(self: "Dataset", x: str, y: str, **kwargs) -> "Bundle":
    """
    Test for equality of variances between two samples.

    By default, performs the standard F test of the null hypothesis that
    the population variances are equal. Robust variants based on Levene
    and Brown-Forsythe tests are also available.

    Parameters
    ----------
    x : str
        First sample.
    y : str, optional
        Second sample. Required unless ``split_by`` is specified.

    Other Parameters
    ----------------
    split_by : str
        Dummy variable defining two samples from ``x``.
    quiet : bool
        Suppress printed output.
    robust : str
        Robust test method: ``"mean"`` for Levene's test, ``"median"``
        for the Brown-Forsythe median-based test, or ``"trimmed"`` for
        the trimmed-mean variant. A trimming percentage may be specified
        as, for example, ``"trimmed,5"``.

    Notes
    -----
    The standard F test assumes normality. Robust alternatives may be
    preferable for non-normal data, particularly the median-based test
    for skewed data and the trimmed-mean test for fat-tailed data.
    """
    return self._test(type="vartest", varlist=x + " " + y, **kwargs)
_vartest.__name__ = "vartest"
_vartest.__qualname__ = "Dataset.vartest"

# #######################################################
#   GretlMatrix class
# #######################################################
def _matrix_to_pandas(self: _gretl.Matrix, view: bool = False) -> pd.DataFrame:
    """
    Convert GretlMatrix to a pandas DataFrame.

    Parameters
    ----------
    view : bool, default False
        If True, construct the DataFrame using a zero-copy NumPy view of the
        underlying matrix data (when possible).
        If False, a deep copy of the matrix data is created.

    Returns
    -------
    pandas.DataFrame
        DataFrame representation of the matrix, with optional row and column
        labels if available.

    Notes
    -----
    When `view=True`, the resulting DataFrame may share memory with the
    underlying GretlMatrix via a NumPy view. However, pandas does not guarantee
    full view semantics internally, so modifications may not always propagate
    back to the original matrix in all cases.
    """

    rows_labels = self.rnameget() if not self.is_complex and len(self.rnameget()) else None
    cols_labels = self.cnameget() if not self.is_complex and len(self.cnameget()) else None

    return pd.DataFrame(data = self.numpy_view if view else self.to_numpy(),
                        index = rows_labels, columns = cols_labels)

def _ones(rows: int, cols: int):
    return _gretl.Matrix(rows, cols, 1.0)
_ones.__name__ = "ones"
_ones.__qualname__ = "Matrix.ones"

if __name__ == "__main__":
    pass
