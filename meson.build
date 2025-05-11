project('gretl4py', 'cpp',
  version : '0.2',
  default_options : ['cpp_std=c++17', 'optimization=2', 'buildtype=release']
)

#########################################################
# c++ flags
#########################################################
cpp = meson.get_compiler('cpp')

# Set common compiler flags
cpp_args = ['-pipe', '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2']

if cpp.get_id() == 'gcc'
  cpp_args += ['-fvisibility=hidden']
endif

# For ms windows, don't add -fPIC
if host_machine.system() != 'windows'
  cpp_args += ['-fPIC']
endif

# Apply the arguments
add_project_arguments(cpp_args, language: 'cpp')


#########################################################
# python to use
#########################################################
python_mod = import('python')
python = python_mod.find_installation()
message('Python used by Meson: ' + python.full_path())

python_version = run_command(python, '-c', 'import sys; print(".".join(map(str, sys.version_info[:3])))', check: true).stdout().strip()
message('Python version: ' + python_version)


#########################################################
# pybind11
#########################################################
pybind = dependency('pybind11', required: false)
if not pybind.found()
  pybind_inc = meson.get_external_property('pybind11_include_dir', '')
  pybind = declare_dependency(
    include_directories: include_directories(pybind_inc)
  )
endif


#########################################################
# libgretl
#########################################################
gretl_dep = dependency('gretl', required: false)
if not gretl_dep.found()
  gretl_inc        = meson.get_external_property('gretl_include', '')
  gretl_inc_glib   = meson.get_external_property('gretl_include_glib', '')
  gretl_inc_config = meson.get_external_property('gretl_include_glibconfig', '')
  gretl_lib        = meson.get_external_property('gretl_lib', '')

  if gretl_inc != '' and gretl_lib != ''
    win_libs = []
    if host_machine.system() == 'windows'
      win_libs = ['-lversion', '-lshlwapi', '-lpathcch', '-lbcrypt']
    endif

    gretl_dep = declare_dependency(
      include_directories: include_directories([gretl_inc, gretl_inc_glib, gretl_inc_config]),
      link_args: ['-L' + gretl_lib, '-lgretl-1.0', '-lxml2', '-lfftw3',
                  '-lglib-2.0', '-lintl', '-lgobject-2.0'] + win_libs
    )
  else
    error('gretl not found and no fallback path provided')
  endif
endif

python_dep = python.dependency()

#########################################################
# installation
#########################################################
# by default, we install in user-space
user_site = run_command(python, '-m', 'site', '--user-site', check: true).stdout().strip()

srcs = files(
  'src/gretl4py.cpp',
  'src/gretl4py_data.cpp',
  'src/gretl4py_data_py11.cpp',
  'src/gretl_calls.cpp',
  'src/gretl4py_model.cpp',
  'src/gretl4py_model_py11.cpp',
  'src/simdjson/simdjson.cpp',
)

gretl4py_dep = declare_dependency(
  include_directories: include_directories('src'),
)

# binary module installation
python.extension_module(
  '_gretl',
  srcs,
  dependencies: [pybind, gretl_dep, python_dep, gretl4py_dep],
  include_directories: include_directories('src'),
  install: true,
  install_dir: user_site + '/gretl'
)

# pure Python module files installation
python_module_files = files(
  'gretl/__init__.py',
  'gretl/gretl4py_addons.py',
  'gretl/gretl4py_classes.py'
)

install_data(
  python_module_files,
  install_dir: user_site + '/gretl'
)
