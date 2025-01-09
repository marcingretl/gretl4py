import sysconfig
import sys
import site
import os
import shutil

PYTHON_VER = float(sysconfig.get_python_version())
if PYTHON_VER < 3.11:
    exit(f'gretl4py requires python 3.11 or higher, but {PYTHON_VER:.2f} was used')

# we setup paths, names and so on
PYTHON_PATH = site.getusersitepackages()
SUFFIX = sysconfig.get_config_var(r'EXT_SUFFIX')
MODULE_PATH = os.path.normpath(f'{PYTHON_PATH:s}/gretl')
MODULE = f'_gretl{SUFFIX:s}'
PYTHON_EXE = sys.executable

if os.path.isdir(MODULE_PATH):
    shutil.rmtree(MODULE_PATH)
os.mkdir(MODULE_PATH)

for file in ["__init__.py", "gretl4py_addons.py", "gretl4py_classes.py"]:
    SRC = os.path.normpath(f'gretl/{file:s}')
    print(f'copying {SRC:s} to {MODULE_PATH:s}')
    shutil.copy(SRC, MODULE_PATH)

SRC = os.path.normpath(f'builds/{PYTHON_VER:.2f}/{MODULE:s}')
print(f'copying {SRC:s} to {MODULE_PATH:s}')
shutil.copy(SRC, MODULE_PATH)

print(f'\nrun \'{PYTHON_EXE:s} -c "import gretl; gretl.about(True)"\' to verify installation')
