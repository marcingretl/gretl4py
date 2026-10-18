#!/usr/bin/env python3
"""Stage gretlmpi and, for wheel builds, a private Linux Open MPI runtime."""

import argparse
import os
import shutil
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
    # Preserve the platform-specific executable extension.
    target = dest / ('gretlmpi.exe' if os.name == 'nt' else 'gretlmpi')
    if args.disable:
        # A no-MPI build must not inherit artifacts from an earlier build.
        target.unlink(missing_ok=True)
        args.stamp.parent.mkdir(parents=True, exist_ok=True)
        args.stamp.touch()
        return
    if args.gretlmpi is None:
        parser.error('--gretlmpi is required unless --disable is set')
    shutil.copy2(args.gretlmpi, target)
    target.chmod(target.stat().st_mode | 0o111)

    if args.mpi_path:
        prefix = args.mpi_path.resolve()
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

    # Meson uses the stamp to track successful staging.
    args.stamp.parent.mkdir(parents=True, exist_ok=True)
    args.stamp.touch()


if __name__ == '__main__':
    main()
