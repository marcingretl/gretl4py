import sys
import os
import sysconfig
import shutil
from setuptools import setup, find_packages

major, minor = sys.version_info[:2]
EXT = sysconfig.get_config_var("EXT_SUFFIX")
build_dir = os.path.join(os.getcwd(), 'builds', f"{major}.{minor}")  # build path
binary_src = os.path.join(build_dir, '_gretl' + EXT)
binary_dst_dir = os.path.join(os.getcwd(), 'gretl')
binary_dst = os.path.join(binary_dst_dir, '_gretl' + EXT)

if not os.path.exists(binary_src):
    print(f"Error: Missing '_gretl' binary at {binary_src} — please build with Meson first.", file=sys.stderr)
    sys.exit(1)

# Ensure destination dir exists
os.makedirs(binary_dst_dir, exist_ok=True)

# Copy binary to gretl/
shutil.copy2(binary_src, binary_dst)

# Define files to pick up
python_files = [
    'gretl4py_addons.py',
    'gretl4py_classes.py',
    '__init__.py',
    f'_gretl{EXT}',
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
    package_data = { 'gretl': python_files },
    include_package_data=False,
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
