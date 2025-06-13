import sys
import os
import sysconfig
from setuptools import setup, find_packages, Extension
import glob


EXT = sysconfig.get_config_var("EXT_SUFFIX")
binary_path = os.path.join(os.getcwd(), 'gretl', '_gretl' + EXT)
dll_dir = os.path.join(os.getcwd(), 'gretl', 'lib')
ext_name = f'gretl._gretl'
ext_path = os.path.join("gretl", f"_gretl{EXT}")

# Check _gretl binary exists in gretl/
if not os.path.exists(binary_path):
    print(f"Error: Missing '_gretl' binary at {binary_path}. Please copy it manually before packaging.", file=sys.stderr)
    sys.exit(1)

# On Windows 64-bit, check DLLs exist in gretl/lib/
if sys.platform == "win32" and sys.maxsize > 2**32:
    if not os.path.isdir(dll_dir):
        print(f"Warning: DLL directory {dll_dir} does not exist. Please copy DLLs manually.", file=sys.stderr)
    else:
        dll_files = glob.glob(os.path.join(dll_dir, '*.dll'))
        if not dll_files:
            print(f"Warning: No DLL files found in {dll_dir}. Please copy DLLs manually.", file=sys.stderr)

# Files inside gretl/ to include
package_files = [
    'gretl4py_addons.py',
    'gretl4py_classes.py',
    '__init__.py',
    f'_gretl{EXT}',
    'lib/*',
    'data/*',
    'tmp/*',
    'examples/*',
    ]

# Include DLL files if on Windows 64-bit and dll_dir exists
if sys.platform == "win32" and sys.maxsize > 2**32 and os.path.isdir(dll_dir):
    dll_names = [os.path.join('lib', os.path.basename(f)) for f in glob.glob(os.path.join(dll_dir, '*.dll'))]
    package_files.extend(dll_names)

# We declare a dummy Extension that points to the existing binary.
# This tricks setuptools into marking the wheel as platform-specific.
ext_modules = [
    Extension(
        ext_name,
        sources=[],  # no sources because you provide pre-built binary
    )
]

setup(
    name='gretl4py',
    version='0.21',
    license='GPL-3.0-or-later',
    description='Python bindings for gretl',
    python_requires='>=3.11',
    author='Marcin Błażejowski',
    author_email='marcin@gretlconference.org',
    packages=find_packages(),
    package_data={
        'gretl': package_files,
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
    ext_modules=ext_modules,
)
