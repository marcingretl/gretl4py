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

"""This module installs all necessary gretl4py files."""

import sysconfig
import site
import shutil

platform = sysconfig.get_platform()
path_check = True
if platform == 'win-amd64':
    GRETL_PATH = r'C:\Program Files\gretl'
elif platform[0:5] == "macos":
    GRETL_PATH = '/Applications/Gretl.app'
elif platform == 'linux-x86_64':
    print("make sure gretl is in your PATH")
    GRETL_PATH = ""
    path_check = False
else:
    exit("unsupported operating system (this installer is for MS Windows or macOS")

# we look up for gretl installation
if not sysconfig.os.path.isdir(sysconfig.os.path.normpath(GRETL_PATH)) and path_check:
    exit(f'gretl4py requires gretl installed in: {GRETL_PATH:s}')

# we verify python min version
PYTHON_MIN = 3.11
PYTHON_MIN_INT = 3110
PYTHON_VER = float(sysconfig.get_python_version())
tmp = sysconfig.get_python_version().split(".")
PYTHON_VER_INT = int(tmp[0])*1000 + int(tmp[1])*10
if PYTHON_VER_INT < PYTHON_MIN_INT:
    exit(f'gretl4py requires python {PYTHON_MIN:.2f} or higher, but {PYTHON_VER:.2f} was used')

# we setup paths, names and so on
PYTHON_PATH = site.getusersitepackages()
SUFFIX = sysconfig.get_config_var(r'EXT_SUFFIX')
MODULE_PATH = sysconfig.os.path.normpath(f'{PYTHON_PATH:s}/gretl')
MODULE = f'_gretl{SUFFIX:s}'
PYTHON_EXE = sysconfig.sys.executable

# we create directories and copy all files
if sysconfig.os.path.isdir(MODULE_PATH):
    shutil.rmtree(MODULE_PATH)
sysconfig.os.makedirs(MODULE_PATH)

for file in ["__init__.py", "gretl4py_addons.py", "gretl4py_classes.py"]:
    SRC = sysconfig.os.path.normpath(f'gretl/{file:s}')
    print(f'copying {SRC:s} to {MODULE_PATH:s}')
    shutil.copy(SRC, MODULE_PATH)

SRC = sysconfig.os.path.normpath(f'builds/{PYTHON_VER:.2f}/{MODULE:s}')
print(f'copying {SRC:s} to {MODULE_PATH:s}')
shutil.copy(SRC, MODULE_PATH)

print(f'\nrun \'{PYTHON_EXE:s} -c "import gretl; gretl.about(True)"\' to verify installation')
