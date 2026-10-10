
"""
Packaging support for gretl4py.

Native extensions are compiled separately using Meson/Ninja.
Setuptools packages the prebuilt binaries without compiling them.
"""

import sys
import sysconfig
from pathlib import Path

from setuptools import setup
from setuptools.dist import Distribution


ROOT = Path(__file__).resolve().parent
PACKAGE_DIR = ROOT / "gretl"
VERSION_FILE = ROOT / "version.cfg"


class BinaryDistribution(Distribution):
    """Mark gretl4py wheels as platform-specific."""

    def has_ext_modules(self):
        return True


def get_version():
    """Read the package version from version.cfg."""
    return VERSION_FILE.read_text(encoding="utf-8").strip()


def check_binary():
    """Ensure that the prebuilt Python extension exists."""
    suffix = sysconfig.get_config_var("EXT_SUFFIX")

    if not suffix:
        raise RuntimeError("Cannot determine Python extension suffix")

    binary = PACKAGE_DIR / f"_gretl{suffix}"

    if not binary.is_file():
        raise RuntimeError(
            f"Missing prebuilt extension: {binary}\n"
            "Compile gretl4py with Meson/Ninja before building a wheel."
        )


# Only require a prebuilt binary when building a wheel.
if "bdist_wheel" in sys.argv:
    check_binary()


setup(
    version=get_version(),
    distclass=BinaryDistribution,
)
