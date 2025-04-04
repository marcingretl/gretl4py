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

import sysconfig
import site as _site
import sys

PATH = _site.getusersitepackages()
platform = sysconfig.get_platform()
if platform == "win-amd64":
    PATH = str(PATH + r'\gretl')
    GDIR = r'C:\Program Files\gretl'
    if 'GRETL_PLUGIN_PATH' not in sysconfig.os.environ:
        sysconfig.os.environ['GRETL_PLUGIN_PATH'] = GDIR + r'\plugins'
    if 'GRETL_DATA_PATH' not in sysconfig.os.environ:
        sysconfig.os.environ['GRETL_DATA_PATH'] = GDIR + r'\data'
    sysconfig.os.add_dll_directory(GDIR)
    currpath = sysconfig.os.environ['PATH']
    if GDIR not in currpath:
        sysconfig.os.environ['PATH'] = GDIR + sysconfig.os.pathsep + currpath
elif platform[0:5] == "macos":
    RES = '/Applications/Gretl.app/Contents/Resources/'
    PLUGDIR = RES + 'lib/plugins'
    if not sysconfig.os.path.exists(PLUGDIR):
        PLUGDIR = RES + 'lib/gretl-gtk2' # we fall back to explicit gtk2-based path
    sysconfig.os.environ['GRETL_PLUGIN_PATH'] = PLUGDIR
    sysconfig.os.environ['GRETL_DATA_PATH'] = RES + 'share/gretl/data'
    sysconfig.os.environ['PATH'] += ':' + str(RES + r'bin')
    PATH = str(PATH + r'/gretl')
else:
    PATH = str(PATH + r'/gretl')
sysconfig.sys.path.append(PATH)

# ###################################
# gretl4py stuff
# ###################################
import _gretl
import gretl4py_addons
import gretl4py_classes

# we import some symbols explicitly
from _gretl import (about, set, version)
from .model_registry import register_model
from gretl4py_classes import (get_data, show_data_status)

# deprecations
def csv2dict (fname, pd=1):
    raise RuntimeError('*** This function is deprecated and was removed. Please use \'gretl.get_data()\' instead. ***\n')

def attach_data (pydict):
    raise RuntimeError('*** This function is deprecated and was removed. Please use \'gretl.get_data()\' instead. ***\n')

def attach_gretl_data (fname):
    raise RuntimeError('*** This function is deprecated and was removed. Please use \'gretl.get_data()\' instead. ***\n')

def get_gretl_data (fname):
    raise RuntimeError('*** This function is deprecated and was removed. Please use \'gretl.get_data()\' instead. ***\n')

def printdata (data, namesonly):
    raise RuntimeError('*** This function is deprecated and was removed. Please use GretlDataset class \'print()\' method instead. ***\n')
# deprecations: END

def __model_initializer(model_name: str, *args, **kwargs):
    """
    Private function called by special method __getattr__
    Return Model class when an unknown function is called to gretl module.
    """
    return gretl4py_classes.Model(model_name, *args, **kwargs)

def __getattr__(fun):
    """
    Called when a gretl.fun is called.
    First search in gretl4py_addons, gretl4py_classes and _gretl. If function
    exist return it.
    Otherwise call __model_initializer
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
        "mpols", "panel", "tsls"]

    return sorted(static_members + dynamic_functions)


if __name__ == "__main__":
    pass
