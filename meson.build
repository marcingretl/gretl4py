project('gretl4py', 'cpp',
  version : '0.2',
  default_options : ['cpp_std=c++17', 'optimization=2', 'buildtype=release']
)

fs = import('fs')
is_windows = host_machine.system() == 'windows'
is_darwin  = host_machine.system() == 'darwin'
is_unix    = not is_windows


#########################################################
# User defined options (see 'meson_options.txt')
#########################################################
PYTHON_EXE  = get_option('PYTHON_EXE')  # Python to use
INSTALL     = get_option('INSTALL')     # local install
STACK_PATH  = get_option('STACK_PATH')  # for Windows only
GRETL_PATH  = get_option('GRETL_PATH')  # for macOS only


#########################################################
# libgretl stack (Windows only)
#########################################################
if is_windows
  STACK_PATH = STACK_PATH.replace('\\', '/')
  if not fs.exists(STACK_PATH)
    error('STACK_PATH not found: ' + STACK_PATH + '\nPlease download the gretl Windows stack from:\n  https://sourceforge.net/projects/gretl/files/mscompile/')
  endif

  gretl_include            = STACK_PATH + '/include'
  gretl_include_glib       = STACK_PATH + '/include/glib-2.0'
  gretl_include_glibconfig = STACK_PATH + '/lib/glib-2.0/include'
  gretl_lib                = STACK_PATH + '/lib'
endif


#########################################################
# Compiler flags
#########################################################
cpp = meson.get_compiler('cpp')
cpp_args = ['-pipe', '-fstack-protector-strong', '-Wp,-D_FORTIFY_SOURCE=2', '-fwrapv']
if cpp.get_id() == 'gcc'
  cpp_args += ['-fvisibility=hidden']
endif
if is_unix
  cpp_args += ['-fPIC']
endif
add_project_arguments(cpp_args, language: 'cpp')


#########################################################
# Python
#########################################################
python_mod = import('python')
python = python_mod.find_installation(PYTHON_EXE)
python_dep = python.dependency()
message('Using Python: ' + python.full_path())


#########################################################
# pybind11
#########################################################
pybind_includes = []
pybind_flags = run_command(python, '-m', 'pybind11', '--includes', check: true).stdout().strip()

foreach flag : pybind_flags.split()
  if flag.startswith('-I')
    pybind_includes += flag.substring(2)
  endif
endforeach

pybind = declare_dependency(
  include_directories: pybind_includes
)


#########################################################
# libgretl dependency
#########################################################
if is_windows
  win_link_args = [
    '-L' + gretl_lib,
    '-lgretl-1.0', '-lxml2', '-lfftw3',
    '-lglib-2.0', '-lintl', '-lgobject-2.0',
    '-lversion', '-lshlwapi', '-lpathcch', '-lbcrypt'
  ]

  gretl_dep = declare_dependency(
    include_directories: [
      gretl_include,
      gretl_include_glib,
      gretl_include_glibconfig
    ],
    link_args: win_link_args
  )
elif is_darwin
  GRETL_PATH = GRETL_PATH.replace('\\', '/')
  gretl_include_paths = [
    GRETL_PATH + '/include',
    GRETL_PATH + '/include/glib-2.0',
    GRETL_PATH + '/lib/glib-2.0/include'
  ]
  gretlld = [
    '-L' + GRETL_PATH + '/lib',
    '-lgretl-1.0', '-lglib-2.0', '-lxml2', '-lfftw3', '-lomp', '-lm'
  ]

  gretl_dep = declare_dependency(
    include_directories: gretl_include_paths,
    link_args: gretlld
  )
else
  gretl_dep = dependency('gretl', required: true)
endif


#########################################################
# Source files
#########################################################
gretl4py_inc = include_directories('src')
srcs = files(
  'src/gretl4py.cpp',
  'src/gretl4py_data.cpp',
  'src/gretl4py_data_py11.cpp',
  'src/gretl_calls.cpp',
  'src/gretl4py_model.cpp',
  'src/gretl4py_model_py11.cpp',
  'src/simdjson/simdjson.cpp',
)


#########################################################
# Path to install
#########################################################
if not INSTALL
    destination = python.get_install_dir()
else
    user_site = run_command(
      python, '-m', 'site', '--user-site',
      check: true
    ).stdout().strip()
    destination = user_site + '/gretl'
endif


#########################################################
# Build the extension and install
#########################################################
if INSTALL
    python.extension_module(
      '_gretl',
      srcs,
      dependencies: [
        pybind,
        gretl_dep,
        python_dep,
        declare_dependency(include_directories: gretl4py_inc)
      ],
      include_directories: gretl4py_inc,
      install: true,
      install_dir: destination
    )
endif


#########################################################
# Python files install
#########################################################
if INSTALL
    install_data(
      files(
        'gretl/__init__.py',
        'gretl/gretl4py_addons.py',
        'gretl/gretl4py_classes.py'
      ),
      install_dir: destination
    )
endif