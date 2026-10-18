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
PYTHON_EXE          = get_option('PYTHON_EXE')
STACK_PATH          = get_option('STACK_PATH')
GRETL_PATH          = get_option('GRETL_PATH')
LOCAL_INST          = get_option('LOCAL_INST')
LLVM_RT_PATH        = get_option('LLVM_RT_PATH')
GRETL_INSTALL_PATH  = get_option('GRETL_INSTALL_PATH')
# MPI staging is optional and independent of the Python extension.
NOMPI               = get_option('NOMPI')
MPI_PATH            = get_option('MPI_PATH')

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
if cpp.get_id() == 'gcc' or cpp.get_id() == 'clang'
  cpp_args += ['-fvisibility=hidden']
endif
if is_unix
  cpp_args += ['-fPIC']
endif
add_project_arguments(cpp_args, language: 'cpp')

###########################################################################
# Linker flags for GCC < 9 (std::filesystem)
###########################################################################
cpp_version = cpp.version()
cpp_major = cpp_version.split('.')[0].to_int()
link_args = []
if cpp.get_id() == 'gcc' and cpp_major < 9
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
    python, '-c', 'import sysconfig; print(sysconfig.get_paths()["platlib"])',
    check: true
  ).stdout().strip()
endif
python_gretl_dir = python_sitepkg_dir / 'gretl'

###########################################################################
# Locate installed gretl runtime resources
###########################################################################
if GRETL_INSTALL_PATH != ''
  gretl_install_root = GRETL_INSTALL_PATH.replace('\\', '/')
else
  gretl_install_root = ''

  if is_windows
    candidates = ['C:/Program Files/gretl']
  elif is_darwin
    candidates = ['/Applications/Gretl.app']
  else
    candidates = ['/usr/local', '/usr']
  endif

  foreach candidate : candidates
    if is_windows
      plugin_dir = candidate / 'plugins'
    elif is_darwin
      plugin_dir = candidate / 'Contents/Resources/lib/gretl-gtk3'
    else
      plugin_dir = candidate / 'lib/gretl-gtk3'
    endif

    if fs.is_dir(plugin_dir)
      gretl_install_root = candidate
      break
    endif
  endforeach
endif

if gretl_install_root == ''
  error('Cannot locate installed gretl. Set -DGRETL_INSTALL_PATH=...')
endif

if is_windows
  gretl_plugins_dir = gretl_install_root / 'plugins'
  gretl_runtime_dir = gretl_install_root
else
  if is_darwin
    gretl_plugins_dir = gretl_install_root / 'Contents/Resources/lib/gretl-gtk3'
  else
    gretl_plugins_dir = gretl_install_root / 'lib/gretl-gtk3'
  endif
endif

if not fs.is_dir(gretl_plugins_dir)
  error('gretl plugins directory not found: ' + gretl_plugins_dir)
endif

message('gretl installation: ' + gretl_install_root)
message('gretl plugins: ' + gretl_plugins_dir)

if is_windows
  message('gretl runtime DLLs: ' + gretl_runtime_dir)
endif

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
  if arch == 'aarch64'
    LLVM_RT_PATH += '/' + 'clang_rt.builtins-aarch64.lib'
  elif arch == 'x86_64'
    LLVM_RT_PATH += '/' + 'clang_rt.builtins-x86_64.lib'
  endif
  win_link_args += [LLVM_RT_PATH]
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
  'src/gretl_calls.cpp',
  'src/gretl4py_bundle.cpp',
  'src/gretl4py_bundle_py11.cpp',
  'src/gretl4py_dataset.cpp',
  'src/gretl4py_dataset_py11.cpp',
  'src/gretl4py_matrix.cpp',
  'src/gretl4py_matrix_py11.cpp',
  'src/gretl4py_utils.cpp',
  'src/gretl4py_model.cpp',
  'src/gretl4py_model_py11.cpp',
  'src/gretl4py_packages.cpp',
  'src/gretl4py_packages_py11.cpp',
  'src/gretl4py_run_hansl.cpp',
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
  install_dir: python_gretl_dir,
)

###########################################################################
# Copy binary to gretl (development and wheel packaging)
###########################################################################
copy_binary_part = custom_target(
  'copy_binary_to_gretl',
  input: ext_mod,
  output: 'copied-stamp',
  command: [
    python, '-c',
    'import pathlib, shutil, sys; src = pathlib.Path(sys.argv[1]); dst = pathlib.Path(sys.argv[2]); dst.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(src, dst); pathlib.Path(sys.argv[3]).touch()',
    '@INPUT@',
    build_output_dir / gretl_mod_filename,
    '@OUTPUT@',
  ],
  build_by_default: true,
)

###########################################################################
# Prepare runtime resources (plugins and DLLs)
###########################################################################
# Copy runtime files into the source package for development and wheel builds.
# Do not remove unrelated files from gretl/lib (e.g., future MPI resources).
runtime_args = [
  '--plugins', gretl_plugins_dir,
  '--destination', meson.project_source_root() / 'gretl/lib',
  '--stamp', '@OUTPUT@',
]

if is_windows
  runtime_args += [
    '--extension', '.dll',
    '--runtime', gretl_runtime_dir,
  ]
else
  runtime_args += ['--extension', '.so']
endif

prepare_runtime = custom_target(
  'prepare_gretl_runtime',
  input: files('scripts/prepare_runtime.py'),
  output: 'runtime-prepared.stamp',
  command: [python, '@INPUT@'] + runtime_args,
  build_by_default: true,
)

###########################################################################
# Stage MPI executables and the private Open MPI runtime (Linux Tier 1)
###########################################################################
# Local installations reuse system MPI; distributable builds stage a private copy.
# NOMPI disables MPI staging without affecting the Python extension.
mpi_args = [
  '--destination', meson.project_source_root() / 'gretl',
  '--stamp', '@OUTPUT@',
]

if NOMPI
  # Remove MPI artifacts from previous builds of this source tree.
  mpi_args += ['--disable']
else
  if is_windows or is_darwin
    error('MPI staging is currently implemented for Linux only; use -DNOMPI=true on this platform')
  endif

  # The gretl executable is supplied by the installed gretl distribution.
  gretlmpi_path = gretl_install_root / 'bin/gretlmpi'
  if not fs.is_file(gretlmpi_path)
    error('gretlmpi not found: ' + gretlmpi_path)
  endif
  mpi_args += ['--gretlmpi', gretlmpi_path]

  if not LOCAL_INST
    # Use an explicit Open MPI prefix for reproducible wheel builds.
    if MPI_PATH == ''
      error('Set -DMPI_PATH=... when building a wheel with MPI')
    endif
    mpi_args += ['--mpi-path', MPI_PATH]
  endif
endif

prepare_mpi = custom_target(
  'prepare_mpi_runtime',
  input: files('scripts/prepare_mpi_runtime.py'),
  output: 'mpi-prepared.stamp',
  command: [python, '@INPUT@'] + mpi_args,
  build_by_default: true,
)

###########################################################################
# Install Python files and resources
###########################################################################
install_data(
  'gretl/__init__.py',
  'gretl/_gretl4py_addons.py',
  'gretl/_gretl4py_classes.py',
  'gretl/_gretl4py_model_api.py',
  'gretl/_plotting.py',
  'gretl/_bundled_addons.txt',
  'gretl/curl-ca-bundle.crt',
  install_dir: python_gretl_dir,
)

###########################################################################
# Install resource directories
###########################################################################
# MPI files are staged by the preparation target; packaging reads the source tree.
foreach dirname : ['lib', 'data', 'examples', 'scripts']
  install_subdir(
    'gretl' / dirname,
    install_dir: python_gretl_dir,
  )
endforeach

###########################################################################
# Install bundled function packages
###########################################################################
bundled_file = meson.project_source_root() / 'gretl/_bundled_addons.txt'

bundled_addons = run_command(
  python,
  '-c',
  'import pathlib, sys; print(*[line.strip() for line in pathlib.Path(sys.argv[1]).read_text(encoding="utf-8").splitlines() if line.strip()], sep=chr(10))',
  bundled_file,
  check: true,
).stdout().strip()

if bundled_addons != ''
  foreach addon : bundled_addons.split('\n')
    addon_path = 'gretl/functions' / addon

    if fs.is_dir(addon_path)
      install_subdir(
        addon_path,
        install_dir: python_gretl_dir / 'functions',
      )
    elif fs.is_file(addon_path)
      install_data(
        addon_path,
        install_dir: python_gretl_dir / 'functions',
      )
    else
      error('Bundled addon not found: ' + addon_path)
    endif
  endforeach
endif

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
