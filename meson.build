project('gretl4py', 'cpp',
  version : '0.2',
  default_options : ['cpp_std=c++17', 'buildtype=release']
)

python = import('python').find_installation('python3')
pybind = dependency('pybind11', required: true)

# Declare common dependencies for all platforms
gretl_dep = dependency('gretl', required: true)
python_dep = dependency('python3', required: true)

# Run the shell command to get the user site directory
user_site = run_command('python3', '-m', 'site', '--user-site', check: true).stdout().strip()

# Default Python installation path
#install_dir = python.get_install_dir()

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

gretl_module = python.extension_module(
    '_gretl',
    srcs,
    dependencies: [pybind, gretl_dep, python_dep, gretl4py_dep],
    include_directories: include_directories('src'),
    install: true,
    install_dir: user_site
)
