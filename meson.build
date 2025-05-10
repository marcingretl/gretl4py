project('gretl4py', 'cpp',
  version : '0.2',
  default_options : ['cpp_std=c++17', 'buildtype=release']
)

python = import('python').find_installation('python3')

# Try finding pybind11
pybind = dependency('pybind11', required: false)
if not pybind.found()
  pybind_inc = meson.get_external_property('pybind11_include_dir', '')
  if pybind_inc != ''
    pybind = declare_dependency(
      include_directories: include_directories(pybind_inc)
    )
  else
    error('pybind11 not found and no include path provided')
  endif
endif

# Try finding gretl via pkg-config (Linux)
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

python_dep = dependency('python3')

# Get user site-packages dir
user_site = run_command('python', '-m', 'site', '--user-site', check: true).stdout().strip()

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

python.extension_module(
  '_gretl',
  srcs,
  dependencies: [pybind, gretl_dep, python_dep, gretl4py_dep],
  include_directories: include_directories('src'),
  install: true,
  install_dir: user_site
)
