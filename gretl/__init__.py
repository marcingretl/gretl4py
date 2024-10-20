"""module import all necessary files"""

import sysconfig
import site as _site
PATH = _site.getusersitepackages()
platform = sysconfig.get_platform()
if platform == "win-amd64":
    PATH = str(PATH + r'\gretl')
    sysconfig.os.add_dll_directory(PATH + r'\DLL')
    try:
        RES = sysconfig.os.environ['GRETL_PLUGIN_PATH'][0:sysconfig.os.environ['GRETL_PLUGIN_PATH'].rindex("\\")]
    except KeyError:
        RES = r'C:\Program Files\gretl'
        sysconfig.os.environ['GRETL_PLUGIN_PATH'] = RES + r'\plugins'
        print("*** debug: GRETL_PLUGIN_PATH is: " + sysconfig.os.environ['GRETL_PLUGIN_PATH'])
    if 'GRETL_DATA_PATH' not in sysconfig.os.environ:
        sysconfig.os.environ['GRETL_DATA_PATH'] = RES + r'\data'
        print("*** debug: GRETL_DATA_PATH is: " + sysconfig.os.environ['GRETL_DATA_PATH'])
    sysconfig.os.add_dll_directory(RES)
    print("*** debug: path to libgretl is: " + RES)
elif platform[0:5] == "macos":
    RES = '/Applications/Gretl.app/Contents/Resources/'
    sysconfig.os.environ['GRETL_PLUGIN_PATH'] = RES + 'lib/gretl-gtk2'
    sysconfig.os.environ['GRETL_DATA_PATH'] = RES + 'share/gretl/data'
    PATH = str(PATH + r'/gretl')
else:
    PATH = str(PATH + r'/gretl')
sysconfig.sys.path.append(PATH)
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
    cfunc = ("about", "set", "printdata", "get_gretl_data", "attach_gretl_data", "attach_data", "sample")

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
