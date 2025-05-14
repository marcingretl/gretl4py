#!/usr/bin/env python3
"""
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
"""

"""This module installs all necessary gretl4py files."""

import os
import sys
import sysconfig
import site
import shutil

# Determine platform and gretl install path
platform = sysconfig.get_platform()
path_check = True
if platform == 'win-amd64':
    GRETL_PATH = r'C:\Program Files\gretl'
elif platform.startswith("macos"):
    GRETL_PATH = '/Applications/Gretl.app'
elif platform == 'linux-x86_64':
    print("make sure gretl is in your PATH")
    GRETL_PATH = ""
    path_check = False
else:
    sys.exit("unsupported operating system (this installer is for MS Windows, macOS, or Linux)")

if not os.path.isdir(os.path.normpath(GRETL_PATH)) and path_check:
    sys.exit(f'gretl4py requires gretl installed in: {GRETL_PATH}')

# Verify Python version
PYTHON_MIN = 3.11
major, minor = sys.version_info[:2]
if (major * 100 + minor) < 311:
    sys.exit(f'gretl4py requires Python {PYTHON_MIN:.2f} or higher, but {major}.{minor} was used')

# Setup directories
PYTHON_PATH = site.getusersitepackages()
SUFFIX = sysconfig.get_config_var('EXT_SUFFIX')
MODULE = f'_gretl{SUFFIX}'
MODULE_PATH = os.path.normpath(f'{PYTHON_PATH}/gretl')

# Resolve source directory (this script's location)
SOURCE_DIR = os.path.dirname(os.path.abspath(__file__))
PYVER = f"{major}.{minor}"
BUILD_DIR = os.path.join(SOURCE_DIR, 'builds', PYVER)

# Clean target directory
if os.path.isdir(MODULE_PATH):
    shutil.rmtree(MODULE_PATH)
os.makedirs(MODULE_PATH)

# Copy .py files from gretl/
for file in ["__init__.py", "gretl4py_addons.py", "gretl4py_classes.py"]:
    SRC = os.path.join(SOURCE_DIR, 'gretl', file)
    print(f'copying {SRC} to {MODULE_PATH}')
    shutil.copy(SRC, MODULE_PATH)

# Copy compiled shared object
SRC = os.path.join(BUILD_DIR, MODULE)
print(f'copying {SRC} to {MODULE_PATH}')
shutil.copy(SRC, MODULE_PATH)

# Verify
print(f'\nVerifying installation...\n')
site.addsitedir(PYTHON_PATH)
import gretl
d = gretl.about(False)
print(d['mod_logo'])
