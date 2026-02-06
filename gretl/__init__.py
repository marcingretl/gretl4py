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
import atexit
import platform
from pathlib import Path

# --- Basic paths and platform info ---
package_dir = os.path.dirname(__file__)
data_dir = os.path.join(package_dir, 'data')
lib_dir = os.path.join(package_dir, 'lib')
user_site = os.path.join(_site.getusersitepackages(), 'gretl')

# --- Set essential environment variables ---
os.environ.setdefault('GRETL_HOME', str(Path(package_dir).resolve()))
os.environ.setdefault('GRETL_DATA_PATH', data_dir)
os.environ.setdefault('GRETL_PLUGIN_PATH', lib_dir)
os.environ.setdefault('CURL_CA_BUNDLE', str(Path(user_site) / "curl-ca-bundle.crt"))

# --- Add lib_dir to PATH if not already present ---
current_path = os.environ.get('PATH', '').split(os.pathsep)
if lib_dir not in current_path:
    os.environ['PATH'] = lib_dir + os.pathsep + os.environ['PATH']

# --- Windows-specific DLL handling ---
if sys.platform == "win32":
    os.add_dll_directory(lib_dir)

# --- Linux-specific OpenBLAS handling ---
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

# ###################################
# gretl4py stuff
# ###################################
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

# 100% safe module cleanup
atexit.register(_gretl._gretl4py_cleanup)

# Explicit symbols import
from ._gretl import (about, version, gretldir)                      # module-related
from ._gretl import (Bundle)                                        # constructors
from ._gretl import (delete, genr, include, pkg, print, set, summary)# libgretl's commands interfaces
from ._gretl import (varlist)
from ._gretl import (pvalue, critical, pdf, cdf, invcdf, corr, cov) # libgretl's functions interfaces
from ._gretl import (get_accessor)                                  # gretl4py specific functions
from .gretl4py_classes import _attrs, _oddsratio, mle, nls, gmm, NLSpec
from .gretl4py_classes import get_data, show_data_status, _to_dataframe
from .gretl4py_classes import _matrix_functions_wrapper, _package_get_help
from .gretl4py_classes import _plotfitted, _plotresiduals
from .gretl4py_addons import print_module_source, fc_print, modeltab

# deprecations
# deprecations: END

# Explicit symbols attach to certain classes
setattr(_gretl.Model, "attrs", _attrs)                      # for listing all object's attributes
setattr(_gretl.Model, "plotfitted", _plotfitted)      # fitted and actual against time
setattr(_gretl.Model, "plotresiduals", _plotresiduals)# residuals against time
setattr(_gretl.Model_logit, "oddsratio", _oddsratio)        # for pretty-printing oddsratios
setattr(_gretl.Dataset, "to_dataframe", _to_dataframe)      # gretl data to Pandas DataFrame converter
setattr(_gretl.Package, "get_help", _package_get_help)      # display help (either PDF or txt)
for fun in ['cdf', 'pdf', 'invcdf', 'pvalue', 'critical']:  # wrapper for matrix-based functions
    setattr(_gretl.Dataset, fun, _matrix_functions_wrapper(func_name=fun))

# Dynamically resolvable estimators
_stable_estimators = ["arima", "ols", "wls", "logit",
    "probit", "poisson", "negbin", "biprobit", "duration",
    "heckit", "logistic", "ar", "ar1", "lad", "garch", "mpols",
    "panel", "tsls", "tobit", "quantreg", "var", "vecm",
    "hsk", "dpanel", "intreg", "midasreg"]

# Direct use of GretlModel for the model instantiation
def __model_initializer(model_name: str, *args, **kwargs):
    """
    Return GretlModel class when an unknown function is called to gretl module.
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
