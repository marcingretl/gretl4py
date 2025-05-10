To compile:

1. Setup
*NIX systems:
meson setup builddir #--buildtype=release

*MS Windows:
meson setup builddir --native-file clang.ini

2. Compilation
meson compile -C builddir #-j4
# ninja -C builddir -j4
meson install -C builddir
