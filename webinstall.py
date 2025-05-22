#!/usr/bin/env python3
import os
import sys
import shutil
import zipfile
import urllib.request
from pathlib import Path
import site
import argparse
import platform

def download_wheel(url, dest_folder):
    dest_folder.mkdir(parents=True, exist_ok=True)
    filename = url.split('/')[-1]
    dest_path = dest_folder / filename
    if dest_path.exists():
        print(f"{filename} already downloaded.")
        return dest_path
    print(f"Downloading {filename} ...")
    try:
        urllib.request.urlretrieve(url, dest_path)
    except Exception as e:
        print(f"Error downloading {filename}: {e}")
        sys.exit(1)
    print(f"Downloaded to {dest_path}")
    return dest_path

def extract_wheel(wheel_path, extract_to):
    extract_to.mkdir(parents=True, exist_ok=True)
    print(f"Extracting {wheel_path.name} ...")
    with zipfile.ZipFile(wheel_path, 'r') as zip_ref:
        zip_ref.extractall(extract_to)
    print(f"Extracted to {extract_to}")

def copy_packages(src_folder, site_packages, packages):
    for pkg in packages:
        src = src_folder / pkg
        dest = site_packages / pkg
        if not src.exists():
            print(f"Warning: {pkg} not found in wheel.")
            continue
        if dest.exists():
            print(f"Removing existing {dest} ...")
            if dest.is_dir():
                shutil.rmtree(dest)
            else:
                dest.unlink()
        print(f"Copying {pkg} to site-packages ...")
        if src.is_dir():
            shutil.copytree(src, dest)
        else:
            shutil.copy2(src, dest)
    print("Copy complete.")

def main():
    # Enforce Linux only
    if platform.system() != "Linux":
        print("This installer script works only on Linux.")
        sys.exit(1)

    parser = argparse.ArgumentParser(description="Install gretl4py wheel into user site-packages without pip.")
    parser.add_argument(
        "--version", default="0.2", help="gretl4py version to install (default: 0.2)"
    )
    args = parser.parse_args()

    version = args.version
    major = sys.version_info.major
    minor = sys.version_info.minor
    py_version = f"cp{major}{minor}-cp{major}{minor}"
    wheel_name = f"gretl4py-{version}-{py_version}-manylinux_2_28_x86_64.whl"
    base_url = "https://netcologne.dl.sourceforge.net/project/gretl/gretl4py/snapshots/"
    wheel_url = base_url + wheel_name

    tmp_whls_dir = Path("/tmp/whls")
    extract_dir = tmp_whls_dir / "extracted"

    wheel_path = download_wheel(wheel_url, tmp_whls_dir)
    extract_wheel(wheel_path, extract_dir)

    user_site = Path(site.getusersitepackages())
    if not user_site.exists():
        print(f"Creating user site-packages directory: {user_site}")
        user_site.mkdir(parents=True, exist_ok=True)
    print(f"User site-packages directory: {user_site}")

    packages_to_copy = [
        "gretl",
        "gretl4py.libs",
        f"gretl4py-{version}.dist-info"
    ]

    copy_packages(extract_dir, user_site, packages_to_copy)

    print(f"gretl4py {version} installation complete. You can now 'import gretl'.")

if __name__ == "__main__":
    main()
