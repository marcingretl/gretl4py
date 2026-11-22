# gretl4py test suite

## Current test map (November 2026)

The repository has two complementary layers. **Core/API tests** live directly
in `tests/` and are collected by `pytest`; **package-plotting integration
fixtures** live in [`plots/tests/`](plots/tests/README.md) and are executed by
the approved `probe_package_batch.py` runner. The plotting audit also has its
own offline `pytest` tests of the runner. Plotting `.inp` samples are **not**
collected by pytest automatically.

| Area | Files | Main purpose |
| --- | --- | --- |
| Matrix indexing and ownership | `test_matrix.py`, `test_matrix_properties.py` | Bounds, copies, slicing, NumPy conversion, property-based cases |
| Bundle ownership and API | `test_bundle.py` | Nested bundles, copies, value lifetime, element handling |
| Dataset operations | `test_dataset_audit.py` | Dataset API contracts: transforms, sample, correlation, PCA, k-means |
| Dataset test dispatch | `test_dataset_test.py` | Bounds checks and econometric tests such as `corrgm` and `runs` |
| Hansl execution | `test_hansl.py` | `run_hansl` / `run_script`, subprocess state isolation |
| gretl function packages | `test_packages.py` | Loading, argument validation, returned-value ownership |
| Plotting audit infrastructure | `plots/test_*.py` | Offline catalog, safety, batch runner, and fixture-selection tests |
| Plotting integration | `plots/tests/priority5`–`priority32` | 250 real package cases; run with `--approved` after reviewing sources |

The older sections below document the August 2026 security-review origins
and subsequent Dataset API expansion. **Not every current test represents a
previously observed bug**: some are functional contract or property-based
checks. The status of the graph-producing package suites is documented in
[`plots/STATUS.md`](plots/STATUS.md) and their thematic index in
[`plots/tests/README.md`](plots/tests/README.md).

From the repository root:

```bash
# Core/API tests (requires a built, loadable gretl extension; hypothesis for
# test_matrix_properties.py).
python3 -m pytest tests/test_*.py -v

# Offline plotting-runner tests (no third-party samples executed).
python3 -m pytest tests/plots/test_*.py -v

# One approved package-plot suite, run separately from pytest.
cd tests/plots
python3 probe_package_batch.py --custom priority32 --approved --continue-on-error
```

### Outputs versus source fixtures

The uploaded test tree contains caches (`__pycache__`, `.pytest_cache`,
`.hypothesis`) and produced plotting artifacts (`package_plot_results`,
timestamped `.plt` files). They are **not test inputs**. Preserve the
hand-reviewed `manifest.csv`, `samples/`, `preload/` and any deliberately
bundled `data/` directories; do not blindly delete or move them.

## Historical notes: August 2026 security-review regressions

The original test layer exists because gretl4py had no automated tests at all before the
2026-08 security review — every one of the dozen-plus real bugs found that
month (buffer overflows, double-frees, leaks, missing bounds checks, a
memory-lifecycle bug that took three attempts to actually fix) was found by
hand, and would have gone unnoticed indefinitely without one. The earliest tests here were **regression tests for specific, verified
bugs from that review**. Functional and property-based tests were added later.

## Running

```bash
cd tests
python3 -m pytest -v
```

The `gretl` package must be installed in the Python environment running
pytest. The tests deliberately do not add the source checkout or `build/` to
`sys.path`. Before running, you can verify the active installation with:

```bash
python3 -c "import gretl; print(gretl.__file__)"
```

Plot tests must use the `gp-executor` bundled with that installed package;
do not copy an executable into the source tree or use an external gptranslate
installation.

Optional, if you want it: `pip install pytest-cov` and add `--cov=gretl` to
see what's actually exercised.

## Layout

| File | Covers |
|---|---|
| `conftest.py` | Installed-package import guard, shared fixtures (`bjg_dataset`, `criteria_package`), `run_in_subprocess()` helper |
| `test_matrix.py` | `GretlMatrix`: negative indexing, `INT_MIN` bounds UB, `get_slice()` leak, missing `operator=`, 0-d NumPy array rejection |
| `test_bundle.py` | `GretlBundle`: the double-free regression from fixing the original leak, copy/deepcopy protocol, matrix-from-array |
| `test_dataset_test.py` | `Dataset.test()`: the `corrgm`/`fractint`/`normtest`/`runs` missing-bounds-check bug |
| `test_packages.py` | `GretlPackage`: the `transfer_ownership()` leak (every package-function call leaked its return value) |
| `test_hansl.py` | `run_hansl`/`run_script`: stack buffer overflow, stale-`fb` use-after-free, `MODEL` lifecycle leak, the original crash scenario |
| `test_dataset_audit.py` | Dataset operations and API contracts added in October 2026 |
| `test_matrix_properties.py` | Hypothesis-driven matrix indexing and construction checks |

## Design decisions worth knowing about

**Installed package only.** `conftest.py` rejects imports from the source
checkout, and `run_in_subprocess()` verifies that each child process imports
the same installed package as its parent. No source/build path injection is
used. The bundled `gp-executor` requirement is separate and should be checked
in the plotting integration tests.

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
