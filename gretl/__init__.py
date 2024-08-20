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
    First search in gretl4py_addons, gretl4py_classes and _gretl. If function
    exist return it.
    Otherwise call __model_initializer
    """
    cfunc = ("about", "set", "printdata", "get_gretl_data", "attach_gretl_data", "attach_data")
    
    if fun in dir(gretl4py_addons):
        return getattr(gretl4py_addons, fun)
    if fun in dir(gretl4py_classes):
        return getattr(gretl4py_classes, fun)
    if fun in dir(_gretl) and fun in cfunc:
        return getattr(_gretl, fun)
    def wrapper(*args, **kwargs):
        return __model_initializer(fun, *args, **kwargs)

    return wrapper

if __name__ == "__main__":
    pass
