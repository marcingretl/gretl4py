#!/usr/bin/env python3
"""Copy installed gretl plugins and Windows runtime DLLs into gretl4py."""

import argparse
import shutil
from pathlib import Path


def matching_files(directory: Path, extension: str, label: str) -> list[Path]:
    if not directory.is_dir():
        raise RuntimeError(f"{label} directory does not exist: {directory}")

    files = sorted(p for p in directory.glob(f"*{extension}") if p.is_file())
    if not files:
        raise RuntimeError(f"No {extension} files found in {label} directory: {directory}")
    return files


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plugins", required=True)
    parser.add_argument("--destination", required=True)
    parser.add_argument("--extension", choices=(".so", ".dll"), required=True)
    parser.add_argument("--runtime", default="")
    parser.add_argument("--stamp", required=True)
    args = parser.parse_args()

    plugins = matching_files(Path(args.plugins), args.extension, "Plugin")
    runtime = (
        matching_files(Path(args.runtime), ".dll", "Runtime")
        if args.runtime else []
    )

    destination = Path(args.destination)
    destination.mkdir(parents=True, exist_ok=True)

    # Copy runtime DLLs first; plugin DLLs take precedence on name collisions.
    for source in runtime:
        shutil.copy2(source, destination / source.name)
    for source in plugins:
        shutil.copy2(source, destination / source.name)

    # Only mark the target complete after every copy has succeeded.
    Path(args.stamp).touch()
    print(f"Copied {len(plugins)} plugins to {destination}")
    if runtime:
        print(f"Copied {len(runtime)} runtime DLLs to {destination}")


if __name__ == "__main__":
    main()
