#!/usr/bin/env python3
"""Opt-in native-memory stress probe for repeated Model.save_yhat() calls.

Run against an installed gretl4py package, for example:
    python3 tests/stress_memory_save_yhat.py --iterations 10000

This script is intentionally not named test_*.py: native-process RSS can
fluctuate with allocators, BLAS runtimes and operating systems, so a fixed
memory threshold is not suitable for the regular cross-platform pytest suite.
"""

import argparse
import contextlib
import gc
import os
import sys


def rss_bytes():
    """Return current/peak process RSS where supported, or None."""
    if sys.platform.startswith("linux"):
        try:
            with open("/proc/self/statm", encoding="ascii") as stream:
                resident_pages = int(stream.read().split()[1])
            return resident_pages * os.sysconf("SC_PAGE_SIZE")
        except (OSError, ValueError, IndexError):
            return None

    if sys.platform == "darwin":
        # On macOS, resource.ru_maxrss is measured in bytes.
        import resource
        return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss

    if sys.platform == "win32":
        # psutil is optional; the functional check still works without it.
        try:
            import psutil
        except ImportError:
            return None
        return psutil.Process().memory_info().rss

    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iterations", type=int, default=10000)
    parser.add_argument("--max-growth-mib", type=float, default=16.0)
    args = parser.parse_args()
    if args.iterations < 1 or args.max_growth_mib < 0:
        parser.error("iterations must be positive and max-growth-mib nonnegative")

    import gretl

    # Create a model using the public estimator factory function.
    dataset = gretl.get_data("bjg", quiet=True)
    model = gretl.ols(formula="lg const g", data=dataset).fit()
    original_k = dataset.k
    name = "stress_yhat"

    # Warm up the allocator and establish a fixed-size dataset.
    with open(os.devnull, "w", encoding="utf-8") as sink:
        with contextlib.redirect_stdout(sink):
            for _ in range(200):
                assert model.save_yhat(name) == name
    assert dataset.k == original_k + 1
    gc.collect()
    before = rss_bytes()

    # Repeated replacements should release temporary native allocations.
    with open(os.devnull, "w", encoding="utf-8") as sink:
        with contextlib.redirect_stdout(sink):
            for _ in range(args.iterations):
                assert model.save_yhat(name) == name
                if dataset.k != original_k + 1:
                    raise AssertionError("save_yhat appended a duplicate series")

    gc.collect()
    after = rss_bytes()
    if before is None or after is None:
        print("PASS: stable dataset size; RSS unavailable on this platform")
        print("Note: native-memory growth was not measured")
        return 0

    growth = (after - before) / (1024 * 1024)
    print(f"RSS before: {before / 1024**2:.2f} MiB")
    print(f"RSS after:  {after / 1024**2:.2f} MiB")
    print(f"RSS growth: {growth:.2f} MiB ({args.iterations} iterations)")
    if growth > args.max_growth_mib:
        raise AssertionError(
            f"RSS growth {growth:.2f} MiB exceeds {args.max_growth_mib:.2f} MiB; "
            "investigate native allocations before calling this a leak"
        )
    print("PASS: dataset stability and configured RSS growth threshold")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
