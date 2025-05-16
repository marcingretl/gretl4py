To compile:

1. Setup
*NIX systems:
meson setup builddir #--buildtype=release

*MS Windows:
meson setup builddir

2. Compilation
meson install -C builddir #-j4
# ninja -C builddir -j4

3. Installation
meson install -C builddir
#ninja -C builddir install


########## setuptools ##########
Remember: before building a whl, copy:
1. binary file from 'builds/3.xx' to 'gretl'
2. (windows only): make sure all dll-s are in 'gretl/dll'
python -m build


########## tweaks ##########
And later:
linux:
auditwheel repair dist/gretl4py-0.2-py3-none-any.whl

macOS:
pip install delocate
delocate-listdeps dist/gretl4py-0.2-*.whl
delocate-wheel -w wheelhouse dist/gretl4py-0.2-*.whl
