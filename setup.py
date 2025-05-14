import glob
import sys
import os
from setuptools import setup, find_packages

exts = glob.glob(os.path.join('gretl', '_gretl.*'))
if not exts:
    print("Error: Missing '_gretl' binary in gretl/ — please build with Meson first.", file=sys.stderr)
    sys.exit(1)

setup(
    name='gretl4py',
    version='0.2',
    license='GPL-3.0-or-later',
    description='Python bindings for gretl',
    python_requires='>=3.11',
    author='Marcin Błażejowski',
    packages=find_packages(),
    package_data={
        'gretl': ['_gretl.*', '*.py'],
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
