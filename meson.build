project('gretl4py', 'cpp',
  meson_version : '>=1.4.0',
  default_options : ['cpp_std=c++17', 'optimization=2', 'buildtype=release']
)

fs = import('fs')
is_windows = host_machine.system() == 'windows'
is_darwin  = host_machine.system() == 'darwin'
is_unix    = not is_windows

###########################################################################
# User-defined options
###########################################################################
PYTHON_EXE  = get_option('PYTHON_EXE')
STACK_PATH  = get_option('STACK_PATH')
GRETL_PATH  = get_option('GRETL_PATH')
LOCAL_INST  = get_option('LOCAL_INST')

###########################################################################
# Windows-specific libgretl
###########################################################################
if is_windows
  arch = host_machine.cpu_family()
  arch_subdir = ''

  if arch == 'x86_64'
    arch_subdir = 'x86_64'
  elif arch == 'aarch64'
    arch_subdir = 'aarch64'
  else
    error('Unsupported architecture: ' + arch)
  endif

  STACK_PATH = STACK_PATH.replace('\\', '/')
  if not fs.exists(STACK_PATH)
    error('STACK_PATH not found: ' + STACK_PATH + '\nPlease download the gretl Windows stack from:\n  https://sourceforge.net/projects/gretl/files/mscompile/')
  endif

  gretl_include            = STACK_PATH + '/include'
  gretl_include_glib       = STACK_PATH + '/include/glib-2.0'
  gretl_include_glibconfig = STACK_PATH + '/lib/' + arch_subdir + '/glib-2.0/include'
  gretl_lib                = STACK_PATH + '/lib/' + arch_subdir
endif

###########################################################################
# Compiler flags
###########################################################################
cpp = meson.get_compiler('cpp')
cpp_args = ['-pipe', '-fstack-protector-strong', '-Wp,-D_FORTIFY_SOURCE=2', '-fwrapv']
if cpp.get_id() == 'gcc'
  cpp_args += ['-fvisibility=hidden']
endif
if is_unix
  cpp_args += ['-fPIC']
endif
add_project_arguments(cpp_args, language: 'cpp')

###########################################################################
# Linker flags for GCC < 9 (std::filesystem)
###########################################################################
link_args = []
if cpp.get_id() == 'gcc' and version_compare(cpp.version(), '<', '9.0')
  message('GCC < 9 detected, linking std::filesystem with -lstdc++fs')
  link_args += ['-lstdc++fs']
endif
add_project_link_arguments(link_args, language: 'cpp')

###########################################################################
# Python
###########################################################################
python_mod = import('python')
python = python_mod.find_installation(PYTHON_EXE)
python_version = python.language_version()
python_major_minor = python_version.split('.').get(0) + '.' + python_version.split('.').get(1)
this_source_dir = meson.current_source_dir()
build_output_dir = this_source_dir / 'gretl'
gretl_mod_suffix = run_command(python, '-c', 'import sysconfig; print(sysconfig.get_config_var("EXT_SUFFIX"))', check: true).stdout().strip()
gretl_mod_filename = '_gretl' + gretl_mod_suffix
python_dep = python.dependency()

###########################################################################
# local (default) or system-wide installation
###########################################################################
if LOCAL_INST
  python_sitepkg_dir = run_command(
    python, '-c', 'import site; print(site.getusersitepackages())',
    check: true
  ).stdout().strip()
else
  python_sitepkg_dir = run_command(
    python, '-c', 'import sysconfig; print(sysconfig.get_paths()[\"platlib\"])',
    check: true
  ).stdout().strip()
endif
python_gretl_dir = python_sitepkg_dir / 'gretl'

###########################################################################
# pybind11
###########################################################################
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

###########################################################################
# libgretl dependency
###########################################################################
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

###########################################################################
# Sources
###########################################################################
gretl4py_inc = include_directories('src')
srcs = files(
  'src/gretl4py.cpp',
  'src/gretl4py_data.cpp',
  'src/gretl4py_utils.cpp',
  'src/gretl4py_data_py11.cpp',
  'src/gretl_calls.cpp',
  'src/gretl4py_model.cpp',
  'src/gretl4py_model_py11.cpp',
  'src/gretl4py_packages.cpp',
)

###########################################################################
# Build the extension
###########################################################################
ext_mod = python.extension_module(
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
  install_dir: python_sitepkg_dir,
)

###########################################################################
# Copy binary to gretl
###########################################################################
python_cmd = [python, '-c', 'import sys, shutil; shutil.copy2(sys.argv[1], sys.argv[2])']
copy_cmd = python_cmd + ['@INPUT@', join_paths(build_output_dir, gretl_mod_filename)]

copy_binary_part = custom_target('copy_binary_to_gretl',
  input: ext_mod,
  output: 'copied-stamp',
  command: copy_cmd,
  build_by_default: true
)

###########################################################################
# Standard install for .py files and resources
###########################################################################
install_data(
  'gretl/__init__.py',
  'gretl/gretl4py_addons.py',
  'gretl/gretl4py_classes.py',
  'gretl/curl-ca-bundle.crt',
  install_dir: python_gretl_dir,
)
meson.add_install_script(python, join_paths(meson.project_source_root(), 'install.py'))

###########################################################################
# Generating 'gretl4py_version.h' header
###########################################################################
version_cmd = run_command(
  python,
  '-c',
  '''
import sys

version_str = open("version.cfg").read().strip()
major_str, minor_str = version_str.split('.')
major = int(major_str)
minor = int(minor_str)

if len(major_str) != 1 or len(minor_str) != 2 or major < 0 or minor < 0:
    sys.exit(1)

print(version_str)
  ''',
  check: false
)

if version_cmd.returncode() != 0
    error('Expected version format: X.yz, where X >= 0 and minor has 2 digits, e.g., 0.10, 1.05')
endif

version_str = version_cmd.stdout().strip()
configure_file(
    input: 'gretl4py_version.h.in',
    output: 'gretl4py_version.h',
    configuration: { 'VERSION': '"' + version_str + '"' }
)
message('Building gretl4py version: ' + version_str)
