import sysconfig
import site

PYTHON_PATH = site.getusersitepackages()
print('gretl4py files were installed to:')
print(sysconfig.os.path.normpath(f'{PYTHON_PATH:s}/gretl'))
