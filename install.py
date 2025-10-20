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
import glob

# Setup directories
PYTHON_PATH = site.getusersitepackages()
SUFFIX = sysconfig.get_config_var('EXT_SUFFIX')
MODULE = f'_gretl{SUFFIX}'
MODULE_PATH = os.path.normpath(f'{PYTHON_PATH}/gretl')
SOURCE_DIR = os.path.dirname(os.path.abspath(__file__))
major, minor = sys.version_info[:2]
PYVER = f"{major}.{minor}"
BUILD_DIR = os.path.join(SOURCE_DIR, 'gretl')

# Copy compiled shared object
SRC = os.path.join(BUILD_DIR, MODULE)
shutil.copy(SRC, MODULE_PATH)

# We copy libs
LIB_DIR = os.path.join(MODULE_PATH, 'lib')
os.makedirs(LIB_DIR, exist_ok=True)

data_files = glob.glob(os.path.join(SOURCE_DIR, 'gretl', 'lib', '*'))

for path in data_files:
    if os.path.isfile(path):
        shutil.copy(path, LIB_DIR)
    elif os.path.isdir(path):
        dest = os.path.join(LIB_DIR, os.path.basename(path))
        if os.path.exists(dest):
            shutil.rmtree(dest)
        shutil.copytree(path, dest)

# We copy datasets
DATA_DIR = os.path.join(MODULE_PATH, 'data')
try:
    os.makedirs(DATA_DIR)
except FileExistsError:
    pass
data_files = glob.glob(os.path.join(SOURCE_DIR, 'gretl', 'data', '*'))
for file in data_files:
    shutil.copy(file, DATA_DIR)

# We copy examples
EXAMPLES_DIR = os.path.join(MODULE_PATH, 'examples')
if os.path.exists(EXAMPLES_DIR):
    shutil.rmtree(EXAMPLES_DIR)
shutil.copytree(os.path.join(SOURCE_DIR, 'gretl', 'examples'), EXAMPLES_DIR)

# We copy addons
ADDONS_DIR = os.path.join(MODULE_PATH, 'functions')
if os.path.exists(ADDONS_DIR):
    shutil.rmtree(ADDONS_DIR)
shutil.copytree(os.path.join(SOURCE_DIR, 'gretl', 'functions'), ADDONS_DIR)

# We add tmp for workdir
try:
    os.makedirs(os.path.join(MODULE_PATH, 'tmp'))
except FileExistsError:
    pass

# We printout summary
print('gretl4py files were installed to:')
print(sysconfig.os.path.normpath(f'{PYTHON_PATH:s}/gretl'))

# We put stamp
with open('resources-installed.stamp', 'w') as f:
    f.write('done\n')
