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
    try:
        from tabulate import tabulate
    except ModuleNotFoundError:
        print('*** warning: oddsratios printing requires \'tabulate\' package ***\n')
        return

    try:
        data = self.oddsratios      # assumed to be a NumPy array
        names = self.parnames[1:]   # skip constant/intercept
        print(tabulate(data, showindex=names,
                       headers=["odds ratio", "std. error", "low95", "high95"],
                       tablefmt="plain"))
    except Exception as e:
        print('*** warning: oddsratios not available for current model ***\n')
        print(f'Debug info: {e}')

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
    if kind == "fitted":
        _plotfitted(self, eqn, multiple, against, backend)
    elif kind == "residuals":
        _plotresiduals(self, eqn, multiple, against, backend)
    else:
        raise ValueError(f"unknown plotting type: {kind}")

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

def _to_dataframe(self: "Dataset") -> pd.DataFrame:
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

# #######################################################
#   GretlMatrix class
# #######################################################
def _to_pandas (self: _gretl.Matrix, view: bool = False) -> pd.DataFrame:
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

    rows_labels = self.rnameget() if len(self.rnameget()) else None
    cols_labels = self.cnameget() if len(self.cnameget()) else None

    return pd.DataFrame(data = self.numpy_view if view else self.to_numpy(),
                        index = rows_labels, columns = cols_labels)

if __name__ == "__main__":
    pass
