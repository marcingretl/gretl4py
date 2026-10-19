#!/usr/bin/env python3
"""Stage gretlmpi and private MPI runtimes for distributable builds."""

import argparse
import os
import shutil
import sys
import subprocess
from pathlib import Path


def patch_elf(path: Path, rpath: str) -> None:
    """Set a relative search path only for ELF executables and libraries."""
    if path.is_symlink() or not path.is_file():
        return
    with path.open('rb') as stream:
        if stream.read(4) != b'\x7fELF':
            return
    subprocess.run(['patchelf', '--set-rpath', rpath, str(path)], check=True)


def copy_directory(source: Path, target: Path) -> None:
    """Copy installed runtime resources, resolving installation symlinks."""
    if source.is_dir():
        shutil.copytree(source, target, dirs_exist_ok=True, symlinks=False)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gretlmpi', type=Path)
    parser.add_argument('--disable', action='store_true')
    parser.add_argument('--destination', type=Path, required=True)
    parser.add_argument('--mpi-path', type=Path)
    parser.add_argument('--stamp', type=Path, required=True)
    args = parser.parse_args()

    dest = args.destination
    dest.mkdir(parents=True, exist_ok=True)
    mpi_dir = dest / 'mpi'

    # Clear any private runtime left by a previous wheel build.
    if mpi_dir.exists():
        shutil.rmtree(mpi_dir)
    # Clear stale executables from builds for either platform.
    for name in ('gretlmpi', 'gretlmpi.exe'):
        (dest / name).unlink(missing_ok=True)
    # Remove an MS-MPI DLL left by an earlier Windows ARM64 wheel build.
    (dest / 'lib' / 'msmpi.dll').unlink(missing_ok=True)
    # Preserve the platform-specific executable extension.
    target = dest / ('gretlmpi.exe' if os.name == 'nt' else 'gretlmpi')
    if args.disable:
        # A no-MPI build must not inherit artifacts from an earlier build.
        args.stamp.parent.mkdir(parents=True, exist_ok=True)
        args.stamp.touch()
        return
    if args.gretlmpi is None:
        parser.error('--gretlmpi is required unless --disable is set')
    shutil.copy2(args.gretlmpi, target)
    target.chmod(target.stat().st_mode | 0o111)

    if args.mpi_path:
        prefix = args.mpi_path.resolve()
        if os.name == 'nt':
            # Windows ARM64: copy only runtime binaries, not MSYS2 development files.
            runtime_files = {
                'lib/msmpi.dll': 'lib/msmpi.dll',
                'bin/mpiexec.exe': 'mpi/bin/mpiexec.exe',
                'bin/smpd.exe': 'mpi/bin/smpd.exe',
            }
            for source, destination in runtime_files.items():
                src = prefix / source
                dst = dest / destination
                if not src.is_file():
                    raise FileNotFoundError(f'MS-MPI runtime file not found: {src}')
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dst)
        elif sys.platform == "darwin":
            stage_openmpi_macos(prefix, mpi_dir, target)
        else:
            stage_openmpi(prefix, mpi_dir, target)

    # Meson uses the stamp to track successful staging.
    args.stamp.parent.mkdir(parents=True, exist_ok=True)
    args.stamp.touch()


def is_macho(path: Path) -> bool:
    if not path.is_file() or path.is_symlink():
        return False
    with path.open('rb') as f:
        return f.read(4) in (
            b'\xfe\xed\xfa\xce', b'\xce\xfa\xed\xfe',
            b'\xfe\xed\xfa\xcf', b'\xcf\xfa\xed\xfe',
            b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe',
            b'\xca\xfe\xba\xbf', b'\xbf\xba\xfe\xca',
        )


def patch_macho(path: Path, libdir: Path) -> None:
    """Redirect bundled MPI library dependencies to their private copies."""
    if not is_macho(path):
        return
    relative_lib = os.path.relpath(libdir, path.parent)
    loader_lib = '@loader_path/' + relative_lib
    output = subprocess.run(['otool', '-L', str(path)], check=True,
                            capture_output=True, text=True).stdout
    for line in output.splitlines()[1:]:
        dep = line.strip().split(' (compatibility version', 1)[0]
        if not dep:
            continue
        name = Path(dep).name
        if (libdir / name).is_file() and dep != loader_lib + '/' + name:
            subprocess.run(['install_name_tool', '-change', dep,
                            loader_lib + '/' + name, str(path)], check=True)
    if path.parent == libdir and path.suffix == '.dylib':
        subprocess.run(['install_name_tool', '-id', '@rpath/' + path.name,
                        str(path)], check=True)


def stage_openmpi_macos(prefix: Path, mpi_dir: Path, target: Path) -> None:
    """Stage either a conventional OpenMPI prefix or Gretl.app's MPI runtime."""
    if not (prefix / 'bin/mpiexec').is_file():
        raise FileNotFoundError(f'OpenMPI launcher not found in {prefix}')
    if not (prefix / 'lib').is_dir():
        raise FileNotFoundError(f'OpenMPI libraries not found in {prefix}')

    for dirname in ('bin', 'lib', 'lib64', 'share', 'etc'):
        copy_directory(prefix / dirname, mpi_dir / dirname)
    for dirname in ('doc', 'man'):
        shutil.rmtree(mpi_dir / 'share' / dirname, ignore_errors=True)
    bindir = mpi_dir / 'bin'
    for entry in bindir.iterdir():
        if entry.name not in ('mpiexec', 'mpirun', 'prterun', 'prted',
                              'orted', 'orterun') and entry.is_file():
            entry.unlink()
    for libname in ('lib', 'lib64'):
        libdir = mpi_dir / libname
        if libdir.is_dir():
            for item in libdir.rglob('*'):
                if item.is_file() and item.suffix in ('.a', '.la'):
                    item.unlink()
            for pkgdir in libdir.rglob('pkgconfig'):
                if pkgdir.is_dir():
                    shutil.rmtree(pkgdir)

    libdir = mpi_dir / 'lib'
    # Rewrite references to bundled libraries, including MCA/PMIx plugins.
    for path in [target, *bindir.rglob('*'), *libdir.rglob('*')]:
        if path.is_file():
            patch_macho(path, libdir)


def stage_openmpi(prefix: Path, mpi_dir: Path, target: Path) -> None:
    """Stage the private Linux Open MPI runtime and patch ELF search paths."""
    if not (prefix / 'bin/mpiexec').exists():
        raise FileNotFoundError(f'Open MPI launcher not found in {prefix}')
    # Preserve PRRTE, PMIx, MCA data and auxiliary executables.
    for dirname in ('bin', 'lib', 'lib64', 'share', 'etc'):
        copy_directory(prefix / dirname, mpi_dir / dirname)

    # Remove documentation not required by the MPI runtime.
    # Keep share/openmpi, share/prrte and share/pmix runtime resources.
    for dirname in ('doc', 'man'):
        shutil.rmtree(mpi_dir / 'share' / dirname, ignore_errors=True)

    # Remove compiler wrappers and unrelated development tools from bin.
    # Preserve runtime libraries and non-ELF PRRTE/PMIx data in lib.
    bindir = mpi_dir / 'bin'
    for entry in bindir.iterdir():
        if entry.name not in ('mpiexec', 'mpirun', 'prterun', 'prted', 'orted'):
            if entry.is_file() or entry.is_symlink():
                entry.unlink()

    # Drop development-only files; preserve shared libraries and runtime data.
    for libname in ('lib', 'lib64'):
        libdir = mpi_dir / libname
        if libdir.exists():
            for item in libdir.rglob('*'):
                if item.is_file() and item.suffix in ('.a', '.la'):
                    item.unlink()
            for pkgdir in libdir.rglob('pkgconfig'):
                if pkgdir.is_dir():
                    shutil.rmtree(pkgdir)

    # Relative ELF lookup paths for bundled launchers and shared libraries.
    for executable in bindir.iterdir():
        patch_elf(executable, '$ORIGIN/../lib:$ORIGIN/../lib64')
    for libname in ('lib', 'lib64'):
        libdir = mpi_dir / libname
        if libdir.exists():
            for item in libdir.rglob('*'):
                if item.is_file():
                    relative = os.path.relpath(libdir, item.parent)
                    patch_elf(item, '$ORIGIN:' + '$ORIGIN/' + relative)

    # gretlmpi finds the private MPI libraries alongside the launcher.
    # Replace build-machine paths; libgretl needs separate wheel repair.
    subprocess.run(['patchelf', '--set-rpath',
                    '$ORIGIN/mpi/lib:$ORIGIN/mpi/lib64', str(target)], check=True)



if __name__ == '__main__':
    main()
