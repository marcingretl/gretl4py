"""Resident-memory measurements for native gretl4py regression tests."""

import sys


def rss_kb():
    """Return process RSS in KiB using the platform-specific available API.

    On Unix, resource.ru_maxrss reports the peak resident set: KiB on Linux
    and bytes on macOS. On Windows use psutil to measure current RSS instead.
    The leak assertions are deliberately loose and check gross growth only.
    """
    if sys.platform == "win32":
        # Windows does not provide the Unix `resource` module; skip only
        # memory-growth assertions if the optional psutil package is absent.
        import pytest

        psutil = pytest.importorskip("psutil", reason="RSS test requires psutil on Windows")
        return psutil.Process().memory_info().rss / 1024

    import resource

    peak = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    return peak / 1024 if sys.platform == "darwin" else peak
