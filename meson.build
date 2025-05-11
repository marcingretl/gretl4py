project('gretl4py', 'cpp',
  version : '0.2',
  default_options : ['cpp_std=c++17', 'optimization=2', 'buildtype=release']
)

# Detect platform
is_windows = host_machine.system() == 'windows'
is_unix    = not is_windows

#########################################################
# C++ compiler flags
#########################################################
cpp = meson.get_compiler('cpp')
cpp_args = ['-pipe', '-fstack-protector-strong', '-Wp,-D_FORTIFY_SOURCE=2', '-fwrapv']

if cpp.get_id() == 'gcc'
  cpp_args += ['-fvisibility=hidden']
endif

# Only on Unix do we need -fPIC
if is_unix
  cpp_args += ['-fPIC']
endif

add_project_arguments(cpp_args, language: 'cpp')

#########################################################
# Python detection
#########################################################
python_mod = import('python')
python     = python_mod.find_installation()
message('Using Python: ' + python.full_path())

python_version = run_command(
  python, '-c',
  'import sys; print(".".join(map(str, sys.version_info[:3])))',
  check: true
).stdout().strip()
message('Python version: ' + python_version)

python_dep = python.dependency()

# Where to install
user_site = run_command(
  python, '-m', 'site', '--user-site',
  check: true
).stdout().strip()

#########################################################
# pybind11 (auto everywhere)
#########################################################
pybind = dependency('pybind11', required: true)

#########################################################
# libgretl
#########################################################
gretl_dep = dependency('gretl', required: false)
if not gretl_dep.found()
  # Expect user to pass -Dgretl_* via meson setup or env
  g_inc        = meson.get_external_property('gretl_include', '')
  g_inc_glib   = meson.get_external_property('gretl_include_glib', '')
  g_inc_cfg    = meson.get_external_property('gretl_include_glibconfig', '')
  g_lib        = meson.get_external_property('gretl_lib', '')

  if g_inc == '' or g_lib == ''
    error('libgretl not found: please set -Dgretl_include and -Dgretl_lib')
  endif

  link_args = ['-L' + g_lib, '-lgretl-1.0', '-lxml2', '-lfftw3', '-lglib-2.0', '-lintl', '-lgobject-2.0']
  if is_windows
    link_args += ['-lversion', '-lshlwapi', '-lpathcch', '-lbcrypt']
  endif

  gretl_dep = declare_dependency(
    include_directories: include_directories([g_inc, g_inc_glib, g_inc_cfg]),
    link_args: link_args
  )
endif

#########################################################
# Build and install
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

python.extension_module(
  '_gretl',
  srcs,
  dependencies: [pybind, gretl_dep, python_dep, declare_dependency(include_directories: gretl4py_inc)],
  include_directories: gretl4py_inc,
  install: true,
  install_dir: user_site + '/gretl'
)

install_data(
  files(
    'gretl/__init__.py',
    'gretl/gretl4py_addons.py',
    'gretl/gretl4py_classes.py'
  ),
  install_dir: user_site + '/gretl'
)
