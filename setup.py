import sys
import os
import sysconfig
from setuptools import setup, find_packages

# Determine Python version
major, minor = sys.version_info[:2]
EXT = sysconfig.get_config_var("EXT_SUFFIX")
PATH = os.path.join(os.getcwd(), 'builds', str(major) + '.' + str(minor))  # Absolute path to 'builds/'

# Full path for the binary (absolute path)
binary_file = os.path.join(PATH, '_gretl' + EXT)

# Check if the binary exists
if not os.path.exists(binary_file):
    print(f"Error: Missing '_gretl' binary at {binary_file} — please build with Meson first.", file=sys.stderr)
    sys.exit(1)

# Get all Python files in the gretl directory manually
python_files = [
    os.path.join('gretl', 'gretl4py_addons.py'),
    os.path.join('gretl', 'gretl4py_classes.py'),
    os.path.join('gretl', '__init__.py')
]

# Setup
setup(
    name='gretl4py',
    version='0.2',
    license='GPL-3.0-or-later',
    description='Python bindings for gretl',
    python_requires='>=3.11',
    author='Marcin Błażejowski',
    packages=find_packages(),
    package_data={
        'gretl': python_files + [binary_file],  # Include both Python files and binary
    },
    include_package_data=True,
    zip_safe=False,
    classifiers=[
        'Programming Language :: Python :: 3',
        'Programming Language :: C++',
        'Operating System :: OS Independent',
        'License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)',
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Topic :: Scientific/Engineering',
    ],
)
