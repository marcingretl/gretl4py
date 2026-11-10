"""
 *
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  gretl4py -- Python bindings for gretl
 *  Copyright (C) 2024 Marcin Błażejowski
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

import os
import sys
import subprocess
import csv
import numpy as np
import inspect
import importlib
from types import ModuleType
from typing import Optional
from typing import List
import gretl
import gretl.examples

def _correctvarnames(names: List[str]) -> List[str]:
    """replace const with const_"""
    # in principle we can have a dict with restricted names and their substitutions
    for i in range(len(names)):
        if names[i] == 'const':
            names[i] = 'const_'

def _pd_to_str_format(pd: int) -> str:
    """produces date string formatter for given periodicity"""
    res = None
    if pd == 1:
        res = '%Y'
    elif pd == 4:
        res = '%Y:%q'
    elif pd == 12:
        res = '%Y:%m'
    elif pd in range(5,8):
        res = '%Y-%m-%d'
    else:
        raise TypeError(f'date formatter for pd={pd:d} not supported')

    return res

def _pd_to_pandas_str_format(pd: int) -> str:
    """produces pandas-style date string formatter for given periodicity"""
    res = None
    if pd == 1:
        res = "Y"
    elif pd == 4:
        res = "Q"
    elif pd == 12:
        res = "M"
    elif pd == 5:
        res = "B"
    elif pd in (6, 7):
        res = "D"

    return res

def _import_example_module(name: str) -> Optional[ModuleType]:
    """
    Dynamically imports a gretl example module by short name.

    Searches only plausible paths based on the gretl.examples
    directory structure:
      - gretl.examples.estimators.<name>
      - gretl.examples.packages.<name>
      - gretl.examples.<name> (top-level examples)

    Parameters:
        name (str): short module name, e.g., "arr"

    Returns:
        module object if found, None if not found
    """
    plausible_paths = [
        f"gretl.examples.estimators.{name}",
        f"gretl.examples.packages.{name}",
        f"gretl.examples.Principles_of_Econometrics_4th_edition.{name}",
        f"gretl.examples.{name}",  # fallback top-level
    ]

    for path in plausible_paths:
        try:
            return importlib.import_module(path)
        except (ModuleNotFoundError, ImportError):
            continue

    return None

def df2dict(dframe, pd: int = 1, with_obs: bool = False):
    '''turns a Pandas DataFrame into a dictionary gretl understands'''
    try:
        import pandas
    except ImportError as e:
        raise ImportError("pandas is required for df2dict()") from e
    d = {}
    obs_dates = []
    varname = []
    varname = dframe.axes[1].tolist()[0+with_obs:]
    mydata = dframe.to_numpy(copy=True)
    if with_obs is True:
        obs_dates = [row[0] for row in mydata]
    else:
        obs_dates = dframe.axes[0].tolist()
    _correctvarnames(varname)
    d['data_array'] = np.array([row[0+with_obs:] for row in mydata], np.double)
    d['varname'] = varname
    d['obs_labels'] = obs_dates
    d['periodicity'] = pd

    return d

def nparray2dict(X: np.ndarray, pd: int = 1, Y: Optional[np.ndarray] = None,
                    with_const: bool = False, const_pos: int = 1):
    '''turns a 'raw' NumPy array into a dictionary gretl understands
    X - matrix with observations on explanatory variables
    Y - vector with observations on endogeneus variable
    with_const - boolean indicating if X contains constant term
    const_pos - column number which holds constant term values
    '''
    if Y is not None and Y.shape[0] != X.shape[0]:
        raise ValueError("X and Y are not conformable!")
    d = {}
    obs_labels = []
    varname = []
    if Y is not None:
        # force Tx1 vector
        Y = np.vstack(Y)
        varname.append("y")
    # we create artificial variables names
    for i in range(with_const, np.shape(X)[1]):
        varname.append("x" + str(i+1-with_const))
    if with_const:
        varname.insert(const_pos, "const_")
    # we create artificial obs labels
    for i in range(np.shape(X)[0]):
        obs_labels.append(i+1)
    _correctvarnames(varname)
    if Y is not None:
        d['data_array'] = np.hstack((Y, X))
    else:
        d['data_array'] = X
    d['varname'] = varname
    d['obs_labels'] = obs_labels
    d['periodicity'] = pd

    return d

def series2dict(Y, pd: int = 1, varname: str = "y") -> dict:
    '''turns a pandas series into a dictionary gretl understands'''
    try:
        import pandas
    except ImportError as e:
        raise ImportError("pandas is required for series2dict()") from e

    if not isinstance(Y, pandas.Series):
        raise Exception("Y is not a pandas.core.series.Series!!!")
    d = {}
    if varname == 'const':
        varname = 'const_'
    mydata = Y.to_numpy(copy=True, dtype=np.double)
    d['data_array'] = mydata
    d['varname'] = [varname]
    try:
        obs_dates = Y.index.to_period()
        d['obs_labels'] = [obs.strftime(_pd_to_str_format(pd)) for obs in obs_dates]
    except (AttributeError, TypeError) as err:
        if err:
            print(f'Warning: {err}, fall back to original labels.')
        d['obs_labels'] = Y.index.to_list()
    d['periodicity'] = pd

    return d

def print_module_source(module: str):
    """
    Prints the full source code of the given imported module.

    Parameters:
        mod: imported module (not a string)
    """
    if module.endswith(".py"):
        module = module[:-3]

    # we try to load module as 'mod'
    mod = _import_example_module(module)
    if mod is None:
        raise ModuleNotFoundError(f"gretl example module '{module}' not found in any plausible path")

    print(f"Successfully imported:\n{mod}\n")
    source_file = inspect.getsourcefile(mod)

    if source_file is None:
        print("Source file not found for the module.")
        return
    with open(source_file, "r") as f:
        code = f.read()
    print(code)

def modeltab(models: "List[GretlModel]", output: str = "print",
                tablefmt: str = "github", stars: bool = True,
                coeff_fmt: str = ".3f", stderr_fmt: str = ".4f"):
    """
    Create a comparison table for multiple GretlModel objects.

    Parameters
    ----------
    models : list
        List of GretlModel objects.
    output : str, optional
        "print" -> print the table, "return" -> return as string (default: "print")
    tablefmt : str, optional
        Table format for tabulate (default: "github")
    stars : bool, optional
        Whether to add significance stars (default: True)
    coeff_fmt : str, optional
        Format string for coefficients (default: ".3f")
    stderr_fmt : str, optional
        Format string for standard errors (default: ".4f")

    Returns
    -------
    str or None
        If output="return", returns the formatted table as a string.
        If output="print", prints the table and returns None.
    """

    try:
        from tabulate import tabulate
    except ModuleNotFoundError:
        print("*** warning: modeltab requires 'tabulate' package ***\n")
        return

    # Headers: use _model_id if present
    headers = ["Variable"] + [
        f"Model {m._ids.get('_model_id', i+1) if hasattr(m, '_ids') else i+1}"
        for i, m in enumerate(models)
    ]

    # Variables: start with first model, then append new ones
    variables = list(models[0].parnames)
    for m in models[1:]:
        for v in m.parnames:
            if v not in variables:
                variables.append(v)

    rows = []

    for var in variables:
        row = [var]
        for m in models:
            if var in m.parnames:
                idx = m.parnames.index(var)
                coeff = float(m.coeff[idx])
                stderr = float(m.stderr[idx])

                # Determine stars only if pvalues exist
                star = ""
                if m.asymptotic:
                    pval = 2 * gretl.pvalue(c="z", x=abs(coeff/stderr))
                else:
                    pval = 2 * gretl.pvalue(c="t", x=abs(coeff/stderr), params=m.df)
                if pval is not None:
                    if pval < 0.01:
                        star = "***"
                    elif pval < 0.05:
                        star = "**"
                    elif pval < 0.10:
                        star = "*"
                cell = f"{format(coeff, coeff_fmt)}{star} ({format(stderr, stderr_fmt)})"
            else:
                cell = ""
            row.append(cell)
        rows.append(row)

    # Summary statistics
    stats = ["n", "Adj. R²", "lnL"]
    for stat in stats:
        row = [stat]
        for m in models:
            if stat == "n":
                val = f"{m.T:.1f}"
            elif stat == "Adj. R²":
                val = f"{m.adj_rsq:.4f}"
            elif stat == "lnL":
                val = f"{m.lnl:.2f}"
            else:
                val = ""
            row.append(val)
        rows.append(row)

    table = tabulate(rows, headers=headers, tablefmt=tablefmt)

    if output == "print":
        print(table)
        return None

    return table

if __name__ == "__main__":
    pass
