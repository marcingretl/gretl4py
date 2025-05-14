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

########## setutools ##########
python -m build

And later:
linux:
auditwheel repair dist/gretl4py-0.2-py3-none-any.whl

ms windows:
pip install wheel
python setup.py bdist_wheel --plat-name=win_amd64

macOS:
pip install delocate
delocate-listdeps dist/gretl4py-0.2-*.whl
delocate-wheel -w wheelhouse dist/gretl4py-0.2-*.whl
