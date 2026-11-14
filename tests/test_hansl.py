"""
run_hansl()/run_script() regression tests - buffer overflow (round-3), stale
'fb' use-after-free (round-3), and the MODEL lifecycle bug that originally
crashed with "corrupted size vs. prev_size while consolidating" /
"free(): invalid size" the first two times it was "fixed", before
destroy_working_model() turned out to be the actual correct API to pair
with allocate_working_model().

These run each scenario in a fresh subprocess (see conftest.run_in_subprocess)
rather than in-process: run_hansl()/run_script() share process-wide C++
globals (fb, nfiles, runit, the working MODEL...), so a test here can look
like it "passed" or "failed" purely because of what an earlier, unrelated
test left behind in the same process - a fresh process is what actually
matches how these bugs originally reproduced.
"""
import resource

import pytest

import gretl

from conftest import run_in_subprocess


def _rss_kb():
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss


def test_basic_execution():
    gretl.run_hansl("scalar q = 1 + 1")


def test_long_include_filename_does_not_crash():
    # previously overflowed a 512-byte stack buffer (MAXLEN) fed to a
    # function documented to need FILENAME_MAX (1024 in this build). The
    # fixed behavior doesn't necessarily raise a Python exception (gretl's
    # own internal bound - "strncat_bounded" - can reject it and let the
    # script continue) - the only real invariant is that the process
    # doesn't crash.
    result = run_in_subprocess(
        'import gretl\n'
        f'gretl.run_hansl(\'include "\' + "A" * 2000 + \'"\')\n'
        'print("SURVIVED")\n'
    )
    assert "SURVIVED" in result.stdout, result.stderr
    assert result.returncode == 0


def test_abort_then_continue_no_use_after_free():
    # a line >= MAXLINE with no newline hits the E_TOOLONG early-exit path,
    # which used to leave the global 'fb' file pointer closed but non-null -
    # the next call would then read through the dangling pointer
    result = run_in_subprocess(
        'import gretl\n'
        'too_long = "scalar x = 1 # " + ("a" * 70_000)\n'
        'try:\n'
        '    gretl.run_hansl(too_long)\n'
        'except Exception:\n'
        '    pass\n'
        'gretl.run_hansl("scalar y = 1")\n'  # must not crash / UAF
        'print("SURVIVED")\n'
    )
    assert "SURVIVED" in result.stdout, result.stderr
    assert result.returncode == 0


def test_model_leak_bounded():
    # allocate_working_model() is called on every gretl_execute_script()
    # invocation; without destroy_working_model() pairing it, each call
    # leaked one MODEL struct permanently
    before = _rss_kb()
    for i in range(3_000):
        gretl.run_hansl(f"scalar y = {i}")
    after = _rss_kb()
    assert after - before < 20_000, f"RSS grew by {after - before} KB across 3k run_hansl() calls"


@pytest.mark.parametrize("length", [1022, 1023, 1024, 1025, 1026])
def test_include_filename_at_exact_filename_max_boundary(length):
    # FILENAME_MAX is 1024 in this build; the fix widened the stack buffer
    # from MAXLEN (512) to exactly FILENAME_MAX. Sweep both sides of that
    # exact boundary, not just "very long" - off-by-one is exactly the kind
    # of thing a boundary-unaware test would miss.
    result = run_in_subprocess(
        'import gretl\n'
        f'name = "A" * {length}\n'
        'gretl.run_hansl(\'include "\' + name + \'"\')\n'
        'print("SURVIVED")\n'
    )
    assert "SURVIVED" in result.stdout, result.stderr
    assert result.returncode == 0


def test_nested_include():
    # exercises push_input_file()/pop_input_file() (the fb/fstack mechanism
    # the stale-fb fix touches) across a real two-level include, not just
    # the single-level abort-then-continue case above
    result = run_in_subprocess(
        'import gretl, tempfile, os, textwrap\n'
        'with tempfile.TemporaryDirectory() as d:\n'
        '    inner = os.path.join(d, "inner.inp")\n'
        '    outer = os.path.join(d, "outer.inp")\n'
        '    with open(inner, "w") as f:\n'
        '        f.write("scalar inner_val = 42\\n")\n'
        '    with open(outer, "w") as f:\n'
        '        f.write(f"include {inner}\\nscalar outer_val = inner_val + 1\\n")\n'
        '    gretl.run_script(outer)\n'
        'print("SURVIVED")\n'
    )
    assert "SURVIVED" in result.stdout, result.stderr
    assert result.returncode == 0


def test_original_crash_scenario():
    # this is the exact sequence that originally crashed: a real estimation
    # (mle-advanced.inp) run via run_script(), then the *same* file run
    # again via run_hansl(f"include {path}") - both share the process-wide
    # working MODEL, and freeing it incorrectly at the end of the first
    # call corrupted the heap by the time the second call tried to use it.
    #
    # Run in a subprocess deliberately: this script defines an internal
    # hansl function ("score") that poisons a pre-existing, unrelated
    # gretl-core quirk affecting *later* package loads in the same process -
    # isolating it here keeps that from bleeding into test_packages.py.
    result = run_in_subprocess(
        'import gretl\n'
        'mle_path = gretl.gretldir + "/scripts/mle-advanced.inp"\n'
        'gretl.run_script(mle_path)\n'
        'gretl.run_hansl(f"include {mle_path}")\n'
        'print("SURVIVED")\n'
    )
    assert "SURVIVED" in result.stdout, result.stderr
    assert result.returncode == 0
