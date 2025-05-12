project('gretl4py', 'cpp',
  version : '0.2',
  #meson_version : '>= 1.8.0',
  default_options : ['cpp_std=c++17', 'optimization=2', 'buildtype=release']
)

fs = import('fs')
is_windows  = host_machine.system() == 'windows'
is_darwin   = host_machine.system() == 'darwin'
is_unix     = not is_windows

#########################################################
# STACK_PATH for Windows
#########################################################
STACK_PATH = 'E:/Marcin/gretlapi/stack_minimal'

if is_windows and not fs.exists(STACK_PATH)
  error('STACK_PATH not found: ' + STACK_PATH + '\nPlease download the gretl Windows stack from:\n  https://sourceforge.net/projects/gretl/files/mscompile/')
endif

gretl_include              = STACK_PATH + '/include'
gretl_include_glib         = STACK_PATH + '/include/glib-2.0'
gretl_include_glibconfig   = STACK_PATH + '/lib/glib-2.0/include'
gretl_lib                  = STACK_PATH + '/lib'

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
# Python and pybind11
#########################################################
python_mod = import('python')
python = python_mod.find_installation()
message('Using Python: ' + python.full_path())

python_dep = python.dependency()

if is_darwin
  # macOS workaround: get correct pybind11 headers from the current Python
  pybind_inc_flags = run_command(python, '-m', 'pybind11', '--includes', check: true).stdout().strip().split()
  pybind_includes = []
  foreach flag : pybind_inc_flags
    pybind_includes += flag.strip('-I')
  endforeach

  pybind = declare_dependency(
    include_directories: include_directories(pybind_includes)
  )
else
  pybind = dependency('pybind11', required: true)
endif

user_site = run_command(
  python, '-m', 'site', '--user-site',
  check: true
).stdout().strip()

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
    include_directories: include_directories([
      gretl_include,
      gretl_include_glib,
      gretl_include_glibconfig
    ]),
    link_args: win_link_args
  )
elif is_darwin
  GPATH = '/Applications/Gretl.app/Contents/Resources'
  GRETL_INCLUDE_PATHS = [
    GPATH + '/include',
    GPATH + '/include/glib-2.0',
    GPATH + '/lib/glib-2.0/include'
  ]
  GRETLLD = [
    '-L' + GPATH + '/lib',
    '-lgretl-1.0', '-lglib-2.0', '-lxml2', '-lfftw3', '-lomp', '-lm'
  ]

  gretl_dep = declare_dependency(
    include_directories: include_directories(GRETL_INCLUDE_PATHS),
    link_args: GRETLLD
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
# Build the extension
#########################################################
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
  install_dir: user_site + '/gretl'
)

#########################################################
# Python files
#########################################################
install_data(
  files(
    'gretl/__init__.py',
    'gretl/gretl4py_addons.py',
    'gretl/gretl4py_classes.py'
  ),
  install_dir: user_site + '/gretl'
)
