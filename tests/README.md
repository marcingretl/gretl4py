# gretl4py test suite

This exists because gretl4py had no automated tests at all before the
2026-08 security review — every one of the dozen-plus real bugs found that
month (buffer overflows, double-frees, leaks, missing bounds checks, a
memory-lifecycle bug that took three attempts to actually fix) was found by
hand, and would have gone unnoticed indefinitely without one. Every test
here is a **regression test for a specific, verified bug from that review**
— not speculative coverage. If it's here, it once broke something real.

## Running

```bash
cd tests
python3 -m pytest -v
```

No install step needed — `conftest.py` puts the repo root and `build/` on
`sys.path` directly, the same way the fixes were verified by hand during
the review. Just make sure the C++ extension is built (`meson compile -C
../build`, or however you normally build it) before running.

Optional, if you want it: `pip install pytest-cov` and add `--cov=gretl` to
see what's actually exercised.

## Layout

| File | Covers |
|---|---|
| `conftest.py` | Path setup, shared fixtures (`bjg_dataset`, `criteria_package`), `run_in_subprocess()` helper |
| `test_matrix.py` | `GretlMatrix`: negative indexing, `INT_MIN` bounds UB, `get_slice()` leak, missing `operator=`, 0-d NumPy array rejection |
| `test_bundle.py` | `GretlBundle`: the double-free regression from fixing the original leak, copy/deepcopy protocol, matrix-from-array |
| `test_dataset_test.py` | `Dataset.test()`: the `corrgm`/`fractint`/`normtest`/`runs` missing-bounds-check bug |
| `test_packages.py` | `GretlPackage`: the `transfer_ownership()` leak (every package-function call leaked its return value) |
| `test_hansl.py` | `run_hansl`/`run_script`: stack buffer overflow, stale-`fb` use-after-free, `MODEL` lifecycle leak, the original crash scenario |

## Design decisions worth knowing about

**Subprocess isolation for `run_hansl`/`run_script` tests.** `test_hansl.py`
runs its scenarios via `run_in_subprocess()` (see `conftest.py`) instead of
calling gretl4py directly in the pytest process. This isn't caution for its
own sake — it's load-bearing. These entry points share process-wide C++
globals (`fb`, `nfiles`, `runit`, the working `MODEL`, ...), so:

- a bug can fail to reproduce if an earlier test already left the global
  state in a particular shape (this happened while writing these tests —
  two "expect an exception" assertions failed only when run *after* another
  hansl test, not in isolation), and
- running `mle-advanced.inp` defines an internal hansl function (`score`)
  that poisons a real, pre-existing, unrelated gretl-core quirk affecting
  *any later* package load in the same process — confirmed to reproduce on
  code from well before this review too, so it's not something introduced
  here, just something the test suite has to route around.

A fresh subprocess per scenario sidesteps both. If you add a new test that
calls `run_hansl`/`run_script`/estimates a model, prefer
`run_in_subprocess()` unless you've specifically checked it doesn't care
about ordering.

**"No leak" tests check RSS, generously, not precisely.** `_rss_kb()` +
`resource.getrusage(...).ru_maxrss` before/after N iterations, asserting the
growth stays under a loose threshold (tens of MB for tens of thousands of
calls). This is deliberately not tight — it's meant to catch "this leaks a
real object every call" (which shows up as clear, sustained growth), not to
detect a few stray bytes. If you need to actually track down a leak, don't
extend the threshold logic here — reach for `valgrind --leak-check=full` or
similar against a standalone repro script instead.

**Fixtures load real data, not synthetic stand-ins.** `bjg_dataset` is
gretl's bundled airline-passengers time series; `criteria_package` is a
small, real, network-free `.gfn` package. Several of the bugs here (the
`Dataset.test()` ones especially) only reproduce against a genuine
time-series dataset — a hand-built cross-section wasn't enough during the
original investigation, so don't swap these for something synthetic
without checking.

## Adding a new regression test

When you fix a bug, add the test *before* or *alongside* the fix if you
can, so you can watch it fail first. Template:

```python
def test_short_description_of_the_bug():
    # one line: what was broken, and what the actual failure looked like
    # (crash / wrong value / silent success when it should have raised)
    ...
    assert <the thing that used to not hold>
```

If it touches `run_hansl`/`run_script`/model estimation, use
`run_in_subprocess()` from `conftest.py` unless you've verified it's
order-independent.

## Known gotcha

gretl's own `PRN`-based printer writes straight to the process's real
stdout (a C-level `FILE*`), not through Python's `sys.stdout` — pytest's
default output capture (`-s` disables it, but even without `-s` you'll see
it in "Captured stdout call" on a failure) handles this fine, but if you
ever pipe test output through something that only intercepts Python-level
`sys.stdout`, gretl's own printed output (variable listings, "Done", etc.)
will still show up directly on the terminal.

## Ideas for next steps (not done yet)

- **`hypothesis`** for property-based edge-case testing — most of the bugs
  here were edge cases (empty lists, 0-d arrays, extreme indices) that a
  generator would find faster than a human enumerating examples by hand.
  `pip install hypothesis` and see the examples discussed alongside this
  suite for a starting point.
- **A C++-level unit test layer** (GoogleTest/Catch2 via meson) for pure
  ownership/lifetime invariants — e.g. asserting `GretlMatrix`'s
  `operator=` produces an independent copy — without needing to go through
  Python at all. Lower priority than the Python layer here, since almost
  every bug found in the review only manifested through the actual Python
  API surface, but worth it for the classes that own raw pointers directly
  (`GretlMatrix`, `GretlBundle`, `GretlPrint`).
- **CI integration** — wire `pytest tests/` into whatever this project ends
  up using for automated builds, so this stops being something you have to
  remember to run by hand.


## October 2026: Dataset API audit

`test_dataset_audit.py` adds parameterized tests from the interactive
Dataset review (run with `python3 -m pytest -v test_dataset_audit.py`):

| Area | Checks |
|---|---|
| Clone / setobs | Changing metadata on a clone; source variables unchanged |
| Lags | String/list overloads; four generated lag series; invalid order |
| Sort | Ascending/descending, string/list; rejection on dated series |
| Dataset modification | `addobs(5)` and default annual-to-quarterly `expand()` |
| Correlation | `corr`, `cov`, `xcorrgm` with both list representations |
| PCA | Both overloads return a populated bundle |
| K-means | Cluster-count and initial-centroid overloads; returned bundle contents |
| K-means ownership | Repeated creation/destruction after `GretlBundle(src, false)` fix |
| Cross-tabulation | `xtab` returns expected 3x3 table including margins |
| Observation bounds | Invalid `to_matrix` ranges raise clean exceptions |
| Sample | Restrict and restore sample |

These are **API contract / functional tests**, except where the test
explicitly describes a known bug (notably `to_matrix` bounds and bundle
ownership). They do not claim that every API behavior was previously broken.

The K-means lifetime loop detects crashes/double-frees but cannot prove
absence of leaks. Use Valgrind/ASan for that.

### Requirements and limitations

- `gretl` must be built and loadable from the repository root/build as
  configured in `conftest.py`.
- The real `denmark` sample dataset must be available through
  `gretl.get_data("denmark", quiet=True)`.
- `test_matrix_properties.py` requires the optional `hypothesis` package.
- Tests added in this update were **not executed in this environment**
  because the compiled `gretl` module is unavailable here. Run them
  against your current local build and report any failures for adjustment.

Run everything from `tests/`:

```bash
python3 -m pytest -v
```

Or just the newly added coverage:

```bash
python3 -m pytest -v test_dataset_audit.py
```
