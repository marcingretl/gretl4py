"""
Shared pytest fixtures for the gretl4py test suite.

Historically, the tests were runnable directly against the local dev build
(build/) with no installation step, mirroring how the module was exercised
throughout the 2026-08 security review: `sys.path.insert(0, repo_root)` +
`sys.path.insert(0, repo_root/build)`.

The tests now use the installed gretl package in the active Python environment.
Neither the source checkout nor build/ is added to sys.path.
"""
import importlib.util
import subprocess
import sys
from pathlib import Path

import pytest

# Import gretl only for fixtures that need the compiled extension.
# The plotting audit's offline unit tests can then run without it.


# Resolve the package location without loading the compiled extension.
# Offline plotting infrastructure tests must also work without gretl installed.
_repo_root = Path(__file__).resolve().parent.parent
_gretl_spec = importlib.util.find_spec("gretl")
_gretl_origin = (
    Path(_gretl_spec.origin).resolve()
    if _gretl_spec is not None and _gretl_spec.origin is not None
    else None
)

# Reject imports from the source checkout when a gretl package is available.
if _gretl_origin is not None and _gretl_origin.is_relative_to(_repo_root):
    raise RuntimeError(
        f"Tests must use installed gretl, not the source checkout: {_gretl_origin}"
    )


def run_in_subprocess(script_body: str, timeout: int = 30) -> subprocess.CompletedProcess:
    """
    Run a snippet of Python in a fresh subprocess using the installed package,
    without prepending the source tree or build/ to sys.path.

    Several gretl4py entry points (run_hansl/run_script in particular) share
    process-wide C++ globals (fb, nfiles, runit, the working MODEL, ...) -
    real bugs in this area only reproduce reliably in a clean process, and
    conversely a fix verified in-process can look broken here purely because
    an *earlier, unrelated* test left shared state behind. Use this instead
    of asserting on in-process behavior for anything touching hansl
    execution.
    """
    # Offline tests do not need gretl; subprocess execution does.
    if _gretl_origin is None:
        raise RuntimeError("run_in_subprocess() requires an installed gretl package")

    # Require the child interpreter to import the same installed package.
    preamble = (
        "from pathlib import Path\n"
        "import gretl\n"
        f"if Path(gretl.__file__).resolve() != Path({str(_gretl_origin)!r}):\n"
        "    raise RuntimeError('Subprocess imported a different gretl package')\n"
    )
    return subprocess.run(
        [sys.executable, "-c", preamble + script_body],
        capture_output=True, text=True, timeout=timeout,
    )


@pytest.fixture
def bjg_dataset():
    """A real time-series dataset (12 years of monthly airline passenger
    data) - used wherever a test needs an actual, non-trivial dataset rather
    than a hand-built one."""
    import gretl
    return gretl.get_data(gretl.gretldir + "/data/bjg.gdt")


@pytest.fixture
def criteria_package():
    """A small, locally-available, network-free package with a public
    matrix-returning function - used for package-loading/execution tests."""
    import gretl
    return gretl.include("criteria")
