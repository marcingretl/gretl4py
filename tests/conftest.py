"""
Shared pytest fixtures for the gretl4py test suite.

Makes the tests runnable directly against the local dev build (build/) with
no installation step, mirroring how the module was exercised throughout the
2026-08 security review: `sys.path.insert(0, repo_root)` +
`sys.path.insert(0, repo_root/build)`.
"""
import os
import sys

_repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, _repo_root)
sys.path.insert(0, os.path.join(_repo_root, "build"))

import subprocess

import pytest
import gretl


def run_in_subprocess(script_body: str, timeout: int = 30) -> subprocess.CompletedProcess:
    """
    Run a snippet of Python in a fresh subprocess with the same sys.path
    setup as this conftest.

    Several gretl4py entry points (run_hansl/run_script in particular) share
    process-wide C++ globals (fb, nfiles, runit, the working MODEL, ...) -
    real bugs in this area only reproduce reliably in a clean process, and
    conversely a fix verified in-process can look broken here purely because
    an *earlier, unrelated* test left shared state behind. Use this instead
    of asserting on in-process behavior for anything touching hansl
    execution.
    """
    preamble = (
        f"import sys; sys.path.insert(0, {_repo_root!r}); "
        f"sys.path.insert(0, {os.path.join(_repo_root, 'build')!r})\n"
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
    return gretl.get_data(gretl.gretldir + "/data/bjg.gdt")


@pytest.fixture
def criteria_package():
    """A small, locally-available, network-free package with a public
    matrix-returning function - used for package-loading/execution tests."""
    return gretl.include("criteria")
