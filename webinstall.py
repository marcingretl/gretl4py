import os
import sys
import shutil
import zipfile
import urllib.request
import argparse
import platform
from pathlib import Path
import site
from zipfile import BadZipFile

# Default and fallback locations for wheel downloads
TMP_WHLS_DIR = Path("/tmp/whls")
DEFAULT_BASE_URL = "https://sourceforge.net/projects/gretl/files/gretl4py/"
FALLBACK_URL = "https://master.dl.sourceforge.net/project/gretl/gretl4py/snapshots/"
VER="0.62"

def download_wheel(url: str, dest_folder: Path) -> Path:
    """
    Downloads a .whl file from the specified URL into dest_folder.
    Returns the path to the downloaded file.
    """
    dest_folder.mkdir(parents=True, exist_ok=True)

    # Extract filename smartly even if URL ends in /download
    if url.endswith("/download"):
        filename = url.split("/")[-2]
    else:
        filename = url.split("/")[-1]

    dest_path = dest_folder / filename

    if dest_path.exists():
        print(f"{filename} already downloaded.")
        return dest_path

    print(f"Downloading {filename} ...")
    try:
        urllib.request.urlretrieve(url, dest_path)
    except Exception as e:
        raise SystemExit(f"Error downloading {filename} from {url}: {e}")

    print(f"Downloaded to {dest_path}")
    return dest_path

def extract_wheel(wheel_path: Path, extract_to: Path):
    """
    Extracts the contents of a wheel file to a directory.
    """
    extract_to.mkdir(parents=True, exist_ok=True)
    print(f"Extracting {wheel_path.name} ...")
    try:
        with zipfile.ZipFile(wheel_path, 'r') as zip_ref:
            zip_ref.extractall(extract_to)
        print(f"Extracted to {extract_to}")
    except BadZipFile:
        print(f"Error: {wheel_path.name} is not a valid zip file.")
        sys.exit(1)

def copy_packages(src_folder: Path, site_packages: Path, packages: list):
    """
    Copies specified packages from src_folder into the user site-packages directory.
    Replaces existing copies if found.
    """
    for pkg in packages:
        src = src_folder / pkg
        dest = site_packages / pkg

        if not src.exists():
            print(f"Warning: {pkg} not found in extracted wheel.")
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
    # Enforce Linux platform only
    if platform.system() != "Linux":
        print("This installer script works only on Linux.")
        sys.exit(1)

    # Remove any previous temporary download directory
    if TMP_WHLS_DIR.exists():
        print(f"Removing existing temporary directory: {TMP_WHLS_DIR}")
        shutil.rmtree(TMP_WHLS_DIR)

    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Install gretl4py wheel into user site-packages without pip.")
    parser.add_argument("--version", default=VER, help="gretl4py version to install (default: {VER})")
    parser.add_argument("--base-url", default=DEFAULT_BASE_URL, help="Base URL for downloading wheels")
    args = parser.parse_args()

    version = args.version
    base_url = args.base_url.rstrip("/")

    # Construct Python version tag used in wheel filenames
    major, minor = sys.version_info.major, sys.version_info.minor
    py_tag = f"cp{major}{minor}"
    wheel_name = f"gretl4py-{version}-{py_tag}-{py_tag}-manylinux_2_28_x86_64.whl"

    # Try versioned URL first
    primary_url = f"{base_url}/v{version}/{wheel_name}/download"
    fallback_url = f"{FALLBACK_URL}{wheel_name}"

    extract_dir = TMP_WHLS_DIR / "extracted"

    try:
        wheel_path = download_wheel(primary_url, TMP_WHLS_DIR)
    except SystemExit:
        print("Falling back to snapshot URL...")
        wheel_path = download_wheel(fallback_url, TMP_WHLS_DIR)

    # Extract the wheel
    extract_wheel(wheel_path, extract_dir)

    # Locate user site-packages
    user_site = Path(site.getusersitepackages())
    if not user_site.exists():
        print(f"Creating user site-packages directory: {user_site}")
        user_site.mkdir(parents=True, exist_ok=True)
    print(f"User site-packages directory: {user_site}")

    # Define packages to install from the wheel
    packages_to_copy = [
        "gretl",
        "gretl4py.libs",
        f"gretl4py-{version}.dist-info"
    ]

    # Clean up old gretl4py-related directories from user_site
    for dst_dir in packages_to_copy:
        dst_path = user_site / dst_dir
        if dst_path.exists():
            print(f"Removing existing {dst_path}")
            if dst_path.is_dir():
                shutil.rmtree(dst_path)
            else:
                dst_path.unlink()

    # Copy files from the extracted wheel into user site-packages
    copy_packages(extract_dir, user_site, packages_to_copy)

    # Clean up temporary files (commented out in case you want to inspect them)
    shutil.rmtree(TMP_WHLS_DIR)

    print(f"gretl4py {version} installation complete. You can now 'import gretl'.")

if __name__ == "__main__":
    main()
