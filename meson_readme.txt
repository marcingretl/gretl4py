To compile:

1. Setup
*NIX systems:
meson setup builddir #--buildtype=release

*MS Windows:
meson setup builddir --native-file clang-windows.ini

2. Compilation
meson install -C builddir #-j4
# ninja -C builddir -j4

3. Installation
meson install -C builddir
#ninja -C builddir install

