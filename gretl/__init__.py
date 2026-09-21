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

"""This module imports all necessary files
and sets some required environmental variables."""

import os
import sys
import site as _site
import sysconfig
import platform
from pathlib import Path

# --- Basic paths and platform info ---
package_dir = os.path.dirname(__file__)
data_dir = os.path.join(package_dir, 'data')
lib_dir = os.path.join(package_dir, 'lib')
user_site = os.path.join(_site.getusersitepackages(), 'gretl')

# --- Set essential environment variables ---
GRETL_HOME = Path(package_dir).resolve()
os.environ.setdefault('GRETL_HOME', str(GRETL_HOME))
os.environ.setdefault('GRETL_DATA_PATH', data_dir)
os.environ.setdefault('GRETL_PLUGIN_PATH', lib_dir)
os.environ.setdefault('CURL_CA_BUNDLE', str(Path(user_site) / "curl-ca-bundle.crt"))

# --- Ensure 'tmp' exists in GRETL_HOME ---
(GRETL_HOME / "tmp").mkdir(exist_ok=True)

# --- Add lib_dir to PATH if not already present ---
current_path = os.environ.get('PATH', '').split(os.pathsep)
if lib_dir not in current_path:
    os.environ['PATH'] = lib_dir + os.pathsep + os.environ['PATH']

# --- Windows-specific DLL handling ---
if sys.platform == "win32":
    os.add_dll_directory(lib_dir)

# --- Locate bundled libgretl in auditwheel-repaired Linux wheels ---
if sys.platform == "linux":
    libs_dir = Path(__file__).parent.parent / "gretl4py.libs"
    if libs_dir.is_dir():
        for file in libs_dir.iterdir():
            if "libgretl" in file.name:
                os.environ.setdefault('GRETL_LIB_NAME', str(file.resolve()))
                break

# --- Ensure user site-packages include gretl ---
if user_site not in sys.path:
    sys.path.insert(0, user_site)

# --- Ensure gretlmpi(.exe) exists
# if sys.platform == "win32":
#     gretlmpi = Path(GRETL_HOME) / "gretlmpi.exe"
# else:
#     gretlmpi = Path(GRETL_HOME) / "gretlmpi"
# if not gretlmpi.exists():
#     raise RuntimeError(f"Missing bundled gretlmpi executable: {gretlmpi}")
# if not gretlmpi.is_file() or not os.access(gretlmpi, os.X_OK):
#     raise RuntimeError(f"Invalid gretlmpi executable: {gretlmpi}")

# ###########################################
# gretl4py stuff
# ###########################################
def vc_redist_message():
    arch = platform.machine().lower()

    if arch == "arm64":
        url = "https://aka.ms/vs/17/release/vc_redist.arm64.exe"
        arch_name = "ARM64"
    else:
        url = "https://aka.ms/vs/17/release/vc_redist.x64.exe"
        arch_name = "x64"

    return (
        f"gretl4py on Windows requires the Microsoft Visual C++ 2022 "
        f"Redistributable ({arch_name}).\n\n"
        f"Download:\n{url}"
    )
try:
    from . import _gretl
except ImportError as e:
    if sys.platform == "win32":
        raise RuntimeError(vc_redist_message()) from e
    raise
from . import gretl4py_addons
from . import gretl4py_classes

# #####################################################
# Explicit symbols import
# #####################################################
from ._gretl import (about, version, gretldir)                              # module-related
from ._gretl import (Bundle, Matrix)                                        # constructors
from ._gretl import (delete, genr, include, pkg, print, set, summary)       # libgretl's commands interfaces
from ._gretl import (varlist, run_script, run_hansl, get_uservar)
from ._gretl import (pvalue, critical, pdf, cdf, invcdf, corr, cov)         # libgretl's functions interfaces
from ._gretl import (get_accessor, tdisagg, kpsscrit)
from .gretl4py_addons import print_module_source, fc_print, modeltab
from .gretl4py_classes import _attrs, mle, nls, gmm, NLSpec
from .gretl4py_classes import get_data, show_data_status
from .gretl4py_classes import _dataset_to_pandas
from .gretl4py_classes import _matrix_functions_wrapper, _package_get_help
from .gretl4py_classes import _model_plots, _matrix_to_pandas, _ones
from .gretl4py_classes import _adf, _bds, _coint, _corrgm, _difftest, _fractint
from .gretl4py_classes import _johansen, _kpss, _levinlin, _meantest, _normtest
from .gretl4py_classes import _runs, _vartest, _add, _bkw, _chow, _cusum, _leverage
from .gretl4py_classes import _omit
# to be deprecated
from .gretl4py_classes import _dataset_old_test, _oddsratio, _to_dataframe

# #####################################################
# deprecations
# #####################################################
# _oddsratio:       to be removed on 0.7
# _to_dataframe:    to be removed on 0.7
# Dataset::test():  to be removed on 0.7
# deprecations: END

# #####################################################
# Explicit symbols attach to certain classes
# #####################################################
for fun in ['cdf', 'pdf', 'invcdf', 'pvalue', 'critical']:  # wrapper for matrix-based functions
    setattr(_gretl.Dataset, fun, _matrix_functions_wrapper(func_name=fun))

# gretl.Model class
setattr(_gretl.Model, "attrs", _attrs)                      # for listing all object's attributes
setattr(_gretl.Model, "plot", _model_plots)                 # wrapper for generating model-based plots
setattr(_gretl.Model, "add", _add)
setattr(_gretl.Model, "bkw", _bkw)
setattr(_gretl.Model, "chow", _chow)
setattr(_gretl.Model, "cusum", _cusum)
setattr(_gretl.Model, "leverage", _leverage)
setattr(_gretl.Model, "omit", _omit)
#
setattr(_gretl.Model_logit, "oddsratio", _oddsratio)        # for pretty-printing oddsratios

# gretl.Dataset class
setattr(_gretl.Dataset, "to_pandas", _dataset_to_pandas)    # gretl data to Pandas DataFrame converter
setattr(_gretl.Dataset, "to_dataframe", _to_dataframe)
setattr(_gretl.Dataset, "test", _dataset_old_test)
setattr(_gretl.Dataset, "adf", _adf)
setattr(_gretl.Dataset, "bds", _bds)
setattr(_gretl.Dataset, "coint", _coint)
setattr(_gretl.Dataset, "corrgm", _corrgm)
setattr(_gretl.Dataset, "difftest", _difftest)
setattr(_gretl.Dataset, "fractint", _fractint)
setattr(_gretl.Dataset, "johansen", _johansen)
setattr(_gretl.Dataset, "kpss", _kpss)
setattr(_gretl.Dataset, "levinlin", _levinlin)
setattr(_gretl.Dataset, "meantest", _meantest)
setattr(_gretl.Dataset, "normtest", _normtest)
setattr(_gretl.Dataset, "runs", _runs)
setattr(_gretl.Dataset, "vartest", _vartest)

# gretl.Package class
setattr(_gretl.Package, "get_help", _package_get_help)      # display help (either PDF or txt)

# gretl.Matrix class
setattr(_gretl.Matrix, "ones", staticmethod(_ones))
setattr(_gretl.Matrix, "pandas_view",
        property(lambda self:
                     _matrix_to_pandas(self, True)))        # view matrix as Pandas DataFrame
setattr(_gretl.Matrix, "to_pandas", _matrix_to_pandas)      # export matrix to Pandas DataFrame

# #####################################################
# Dynamically resolvable estimators
# #####################################################
_stable_estimators = ["arima", "ols", "wls", "logit",
    "probit", "poisson", "negbin", "biprobit", "duration",
    "heckit", "logistic", "ar", "ar1", "lad", "garch", "mpols",
    "panel", "tsls", "tobit", "quantreg", "var", "vecm",
    "hsk", "dpanel", "intreg", "midasreg"]

# #####################################################
# Direct use of gretl.Model for the model instantiation
# #####################################################
def __model_initializer(model_name: str, *args, **kwargs):
    """
    Return gretl.Model class when an unknown function is called to gretl module.
    """
    model_class = getattr(_gretl, model_name, None)
    if model_class:
        return model_class(*args, **kwargs)
    else:
        raise AttributeError(f"Model {model_name} not found in _gretl module.")

def __getattr__(fun):
    """
    Called when a gretl.fun is called.
    """
    if fun in dir(gretl4py_addons):
        return getattr(gretl4py_addons, fun)
    if fun in dir(gretl4py_classes):
        return getattr(gretl4py_classes, fun)

    def wrapper(*args, **kwargs):
        return __model_initializer(fun, *args, **kwargs)

    return wrapper

def __dir__():
    """
    Extend the module's __dir__() so that dynamic model
    functions appear in auto-completion.
    """
    return sorted(list(globals().keys()) + _stable_estimators)

if __name__ == "__main__":
    pass
