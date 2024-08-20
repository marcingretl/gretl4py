"""module import all necessary files"""

import sys
import site
import os
PATH = site.getusersitepackages()
if os.name == 'nt':
    PATH = str(PATH + r'\gretl')
    os.add_dll_directory(PATH + r'\DLL')
    os.add_dll_directory(r'C:\Program Files\gretl')
else:
    PATH = str(PATH + r'/gretl')
sys.path.append(PATH)
import _gretl
import gretl4py_addons
import gretl4py_classes

def __model_initializer(model_name: str, *args, **kwargs):
    """
    Private function called by special method __getattr__
    Return Model class when an unknown function is called to gretl module.
    """
    return gretl4py_classes.Model(model_name, *args, **kwargs)

def __getattr__(fun):
    """
    Called when a gretl.fun is called.
    First search in gretl4py_addons and gretl4py_classes. If function
    exist return it.
    Otherwise call __model_initializer
    """
    if fun in dir(gretl4py_addons):
        return getattr(gretl4py_addons, fun)
    if fun in dir(gretl4py_classes):
        return getattr(gretl4py_classes, fun)
    def wrapper(*args, **kwargs):
        return __model_initializer(fun, *args, **kwargs)

    return wrapper

def about(print=True):
    return _gretl.about(do_print=print)

def set(key, val=None):
    if val is None:
        _gretl.set(key)
    else:
        _gretl.set(key, val)

def printdata(data=None, varamesonly=False):
    if data is None:
        _gretl.printdata(varamesonly)
    else:
        _gretl.printdata(data, varamesonly)

def get_gretl_data(fname):
    return _gretl.get_gretl_data(fname)

def attach_gretl_data(fname):
    _gretl.attach_gretl_data(fname)

def attach_data(data_dict):
    _gretl.attach_data(data_dict)

if __name__ == "__main__":
    pass
