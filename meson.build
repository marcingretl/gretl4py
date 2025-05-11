project('gretl4py', 'cpp',
  version : '0.2',
  default_options : ['cpp_std=c++17', 'optimization=2', 'buildtype=release']
)

# get the C++ compiler
cpp = meson.get_compiler('cpp')

if cpp.get_id() == 'gcc'
    add_project_arguments('-pipe', language: 'cpp')
elif cpp.get_id() == 'clang'
    add_project_arguments('-pipe', '-fvisibility=hidden', language: 'cpp')
endif

# the Python binary to use
python = import('python').find_installation('/usr/bin/python3')
message('Python used by Meson: ' + python.full_path())

# Get the actual Python version (for logging/debug)
python_version = run_command(python, '-c', 'import sys; print(".".join(map(str, sys.version_info[:3])))', check: true).stdout().strip()
message('Python version: ' + python_version)

# pybind11 flags (via pkg-config preferred)
pybind = dependency('pybind11', required: false)
if not pybind.found()
  pybind_inc = run_command(python, '-m', 'pybind11', '--includes', check: true).stdout().strip().split()
  pybind = declare_dependency(
    include_directories: include_directories(pybind_inc)
  )
endif

# libgretl (via pkg-config preferred)
gretl_dep = dependency('gretl', required: false)
if not gretl_dep.found()
  gretl_inc = meson.get_external_property('gretl_include', '')
  gretl_lib = meson.get_external_property('gretl_lib', '')
  gretl_inc_glib = meson.get_external_property('gretl_include_glib', '')
  gretl_inc_glibconfig = meson.get_external_property('gretl_include_glibconfig', '')
  if gretl_inc != '' and gretl_lib != ''
    gretl_dep = declare_dependency(
      include_directories: include_directories([gretl_inc, gretl_inc_glib, gretl_inc_glibconfig]),
      link_args: ['-L' + gretl_lib, '-lgretl-1.0', '-lxml2', '-lfftw3',
                  '-lglib-2.0', '-lintl', '-lgobject-2.0',
                  '-lversion', '-lshlwapi', '-lpathcch', '-lbcrypt']
    )
  else
    error('gretl not found and no fallback path provided')
  endif
endif

# Python dependency (headers, etc.)
python_dep = python.dependency()

# Get user site-packages dir (for install)
user_site = run_command(python, '-m', 'site', '--user-site', check: true).stdout().strip()

# Sources
srcs = [
  'src/gretl4py.cpp',
  'src/gretl4py_data.cpp',
  'src/gretl4py_data_py11.cpp',
  'src/gretl_calls.cpp',
  'src/gretl4py_model.cpp',
  'src/gretl4py_model_py11.cpp',
  'src/simdjson/simdjson.cpp',
]

gretl4py_dep = declare_dependency(
  include_directories: include_directories('src'),
)

# Extension module
python.extension_module(
  '_gretl',
  srcs,
  dependencies: [pybind, gretl_dep, python_dep, gretl4py_dep],
  include_directories: include_directories('src'),
  install: true,
  install_dir: user_site
)
