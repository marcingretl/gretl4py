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
from pathlib import Path

# --- Basic paths and platform info ---
package_dir = os.path.dirname(__file__)
data_dir = os.path.join(package_dir, 'data')
lib_dir = os.path.join(package_dir, 'lib')
user_site = os.path.join(_site.getusersitepackages(), 'gretl')

# --- Set essential environment variables ---
os.environ.setdefault('GRETL_HOME', package_dir)
os.environ.setdefault('GRETL_DATA_PATH', data_dir)
os.environ.setdefault('GRETL_PLUGIN_PATH', lib_dir)

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
from . import _gretl
from . import gretl4py_addons
from . import gretl4py_classes

# we import some symbols explicitly
from ._gretl import (about, set, version)
from .gretl4py_classes import attrs, oddsratio
from .gretl4py_classes import get_data, show_data_status
from .gretl4py_addons import print_module_source, fc_print

# deprecations
def csv2dict(fname, pd=1):
    raise RuntimeError('*** This function is deprecated and was removed. Please use \'gretl.get_data()\' instead. ***\n')

def attach_data(pydict):
    raise RuntimeError('*** This function is deprecated and was removed. Please use \'gretl.get_data()\' instead. ***\n')

def attach_gretl_data(fname):
    raise RuntimeError('*** This function is deprecated and was removed. Please use \'gretl.get_data()\' instead. ***\n')

def get_gretl_data(fname):
    raise RuntimeError('*** This function is deprecated and was removed. Please use \'gretl.get_data()\' instead. ***\n')

def gretl_data_path(fname):
    raise RuntimeError('*** This function is deprecated and was removed. Please use /path/to/file directly. ***\n')

def printdata(data, namesonly):
    raise RuntimeError('*** This function is deprecated and was removed. Please use GretlDataset class \'print()\' method instead. ***\n')
# deprecations: END

# we attach additional symbols to certain classes
setattr(_gretl.Model, "attrs", attrs)               # method for listing all object's attributes
setattr(_gretl.Model_logit, "oddsratio", oddsratio) # method for pretty-printing oddsratios

# directly use GretlModel for the model instantiation
def __model_initializer(model_name: str, *args, **kwargs):
    """
    Return GretlModel class when an unknown function is called to gretl module.
    """
    model_class = getattr(_gretl, model_name, None)
    if model_class:
        return model_class(*args, **kwargs)
    else:
        raise AttributeError(f"Model {model_name} not found in _gretl module.")

# simplified __getattr__ to handle dynamic method calls
def __getattr__(fun):
    """
    Called when a gretl.fun is called.
    """
    cfunc = ("about", "set")

    if fun in dir(gretl4py_addons):
        return getattr(gretl4py_addons, fun)
    if fun in dir(gretl4py_classes):
        return getattr(gretl4py_classes, fun)
    if fun in dir(_gretl) and fun in cfunc:
        return getattr(_gretl, fun)

    def wrapper(*args, **kwargs):
        return __model_initializer(fun, *args, **kwargs)

    return wrapper

def __dir__():
    """
    Extend the module's __dir__() so that dynamic model
    functions appear in auto-completion.
    """
    static_members = list(globals().keys())

    # dynamically resolvable functions
    dynamic_functions = ["arima", "ols", "wls", "logit", "probit", "poisson", "negbin",
        "biprobit", "duration", "heckit", "logistic", "ar", "ar1", "lad", "garch",
        "mpols", "panel", "tsls", "tobit", "quantreg", "var", "vecm", "hsk", "dpanel",
        "intreg", "midasreg"]

    return sorted(static_members + dynamic_functions)

if __name__ == "__main__":
    pass
