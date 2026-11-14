# gretl function-package plotting audit (stage 1)

This is a **static inventory and manual integration-test toolkit** for discovering
plot-producing gretl packages. It does not claim runtime compatibility merely
because a plotting keyword appears in a package.

## Sources

- Contributed catalog: <https://gretl.sourceforge.net/cgi-bin/gretldata.cgi>
- Raw files: <https://gretl.sourceforge.net/current_fnfiles/>
- Official addon index: <https://gretl.sourceforge.net/addons-data/addons.xml>
- Official source mirror: <https://github.com/gretl-project/gretl/tree/master/addons>

The official add-on XML index may not directly contain download URLs. The fetcher
tries the official repository's addon files as an additional source. For full
coverage also provide your **local installed addon directory** to `scan --extra`.
The contributed and official inventories are separate, and historical/archived
packages are outside the default scope.

## Local configuration

Edit **`audit_config.py`** to change the base directory. The default is:

```python
PROJECT_ROOT = Path("/home/marcin/cvs/Gretl/gretl-gretl4py/tests/plots/gretl_package_plot_audit")
```

Downloaded contributed packages are kept in `PROJECT_ROOT/audit/contributed/`.
Plots from the manual probe are saved under `PROJECT_ROOT/package_plot_results/`.
Both the fetcher and scanner default to `PROJECT_ROOT/audit/`, so `--root audit`
is no longer needed when using the configured location. Explicit `--root` still works.

## 1. Retrieve package source files (Python 3.11+, no extra packages)

```bash
python package_plot_audit.py fetch
```

To smoke test downloads first:

```bash
python package_plot_audit.py fetch --limit 5
```

This obtains `.gfn` and `.zip` files from the contributed catalog, then attempts
to retrieve official addons. Downloads are cached, use atomic writes, and are
listed in `audit/downloads.csv`. A failed download is explicitly recorded; it
is not silently treated as inspected.

## 2. Inspect without executing anything

```bash
python package_plot_audit.py scan
```

Include the packaged addons already installed on your machine:

```bash
python package_plot_audit.py scan --extra /usr/share/gretl/functions /usr/share/gretl/addons
```

Replace the example addon paths with actual ones (they vary by distribution).
The scanner reads `.gfn` (XML) and selected source/sample files inside `.zip`
without extracting archive contents. Output:

- `plot_packages.csv` — package-level categories and evidence counts.
- `plot_evidence.csv` — **function name, source file, logical source line,
  command and its detection category**, for targeted review.
- `plot_candidates.json` — packages needing follow-up.
- `downloads.csv` — retrieval success/failure, URLs, origin.

### Categories

- `plot_detected`: one or more literal plotting commands found in source
  (`gnuplot`, `boxplot`, `scatters`, `gridplot`, etc.) or explicit `--plot`
  on selected native gretl commands.
- `gnuplot_code_candidate`: constructs associated with manually assembled
  gnuplot scripts, for review.
- `review_required`: generic graphing words found; may be documentation or
  false positives.
- `no_plot_detected`: no **static evidence** found, which does **not** prove
  plotting is impossible (dynamic output, indirect calls, dependencies).

Logical source line numbers in `.gfn` refer to the extracted function code,
not the physical XML file line number. Some old packages have different XML
structures and are marked via `parse_errors` / `sections`.

## 3. Create real integration tests

Study the package's sample script and the function signatures before running.
The manual probe first calls `gretl.include(name)`. If the package is represented
by a `.gfn` or `.zip` file in the **already downloaded** `audit/contributed/`
folder and `include()` raises `FileNotFoundError`, the probe installs that
local file via `gretl.pkg("install", str(path), local=True)` and calls
`include()` again. **It never downloads a package on its own.** Official
addons without contributed cache files keep the original `include()` path.

For a **reviewed, approved** package, the optional manual probe is:

```bash
python probe_package_sample.py SVAR --approved --out svargallery

# Force the addon route, even if a same-named contributed file is cached.
python probe_package_sample.py SVAR --source addon --approved

# Install locally if needed, then run the contributed package's sample.
python probe_package_sample.py PairPlot --source contributed --approved

# Same behavior automatically for cached contributed packages.
python probe_package_sample.py PairPlot --approved
```

Both official addons and contributed packages now search installed `examples/`
**recursively**. A conventional top-level `Package_sample.inp` still uses
`pkg.run_sample()`, while other scripts are executed with `gretl.run_script(path)`.
Nested paths (e.g. `SVAR/examples/Traditional/BlQuah.inp`) are retained in the
list, selector and CSV report. For examples that explicitly open existing data
using `open examples/...`, the probe temporarily changes the Python cwd to the
installed package root **and synchronizes libgretl's `$workdir` with
`set workdir`** before executing the script. The Python cwd is restored
thereafter; each batch example runs in a separate process, so the libgretl
setting cannot leak into other batch examples. Other examples keep their
existing working-directory behavior. This is intentionally selective: a
global working-directory change could break examples that use different
relative-path conventions. If several examples exist, select one explicitly
instead of guessing:

```bash
# Inspect installed samples without executing package code.
python probe_package_sample.py PairPlot --list-samples

# Run one reviewed and approved PairPlot example.
python probe_package_sample.py PairPlot --sample iris1.inp --approved

# List recursive examples and run a reviewed addon sample.
python probe_package_sample.py SVAR --list-samples
python probe_package_sample.py SVAR --source addon \
    --sample SetRestrictions/spaghetti_plot.inp --approved

# A unique basename is also accepted; ambiguous names require the full relative path.
python probe_package_sample.py SVAR --sample BlQuah --approved

# Use a nonstandard installed examples location when necessary.
python probe_package_sample.py PairPlot --examples-dir /path/to/examples --sample growth.inp --approved
```

The `--list-samples` mode is read-only and does not install or include packages;
`--approved` is still required for execution. For contributed packages, an
existing installation is reused; a missing installation is created **only from
the downloaded local archive**. The probe then saves active Matplotlib figures
if any. `--approved` is mandatory to protect against
running arbitrary contributed code. The packaged sample might perform unrelated
work or not request plotting at all; no figures from this test is **not** proof
of no plotting. Some samples need external data, extra packages or an active
network connection. Also, this exploratory probe is not equivalent to invoking
a selected plotting function via `pkg.function(...)` with validated arguments.

Actual long-term tests should be **hand-selected** from `plot_evidence.csv`:
create a fixture for each genuinely graphical public function, use reproducible
datasets, call it with stable parameters, preserve the generated `.gp` on
failure, and render the resulting Python in `Agg` while checking figure data.
Only then can failures in `gptranslate` be classified and fixed systematically.

## Safety and limits

Scanning never runs packages or contacts domains beyond the configured official
repositories. Running sample scripts is an opt-in, separate action. Don't run
unreviewed samples in a privileged environment; they can write files, invoke
external programs or use the network. Scans use keyword heuristics, and therefore
are neither a full hansl call graph nor a dynamic rendering proof.

## Local self-test

```bash
python -m unittest discover -p 'test_*.py' -q
```

The most recent reported run executed 44 tests successfully (14 November 2026).
Some tests intentionally print `[FAIL]` or native error messages while checking
error handling; the `unittest` result (`OK` or `FAILED`) determines whether the
test suite passed. That run preceded the final `set workdir` adjustment, which
was subsequently verified by a focused BVAR example run. Rerun the unit tests
after any further code changes.

## 4. Batch probe: isolated examples, stop on first error

`probe_package_batch.py` runs each approved `.inp` example in **its own Python
process**, one after another. This is important because packages may call
`clear --all`, overwrite global gretl state, or create many plots. By default,
the batch stops on the first failure, while saving full logs and a CSV report.

First review the samples you intend to execute: their code may read/write files,
use the network, or run external programs. `--approved` is mandatory. None of
these commands re-downloads a contributed package: the existing probe installs
from the local `audit/contributed/` cache only when needed.

```bash
# PairPlot: run installed examples in alphabetical order, stop on first error.
python probe_package_batch.py PairPlot --approved

# Reproduce the two examples that exposed missing gptranslate commands.
python probe_package_batch.py PairPlot --samples iris1.inp growth.inp --approved

# Keep going after failures to gather a larger set of diagnostic logs.
python probe_package_batch.py PairPlot --approved --continue-on-error

# Nested addon examples; each runs in an isolated process.
python probe_package_batch.py SVAR --source addon \
    --samples SetRestrictions/spaghetti_plot.inp --approved

# Run all 20 installed SVAR examples ONLY after checking their source code,
# data dependencies and possible file-writing side effects.
python probe_package_batch.py SVAR --source addon --approved --continue-on-error

# Review audit/plot_packages.csv before approving a group of packages.
# Limit this first run to 5 downloaded contributed plotting candidates.
python probe_package_batch.py --candidates --contributed-only --limit 5 --approved

# Run all scanned contributed candidates after reviewing their sample code.
python probe_package_batch.py --candidates --contributed-only --approved
```

### Full candidate batch and focused BVAR regression

Following review of the installed examples, the 95-candidate exploratory pass
was run as:

```bash
python probe_package_batch.py --candidates \
    --approved --continue-on-error --skip-bin
```

The **14 November 2026 full-run snapshot** reported 54 `PASS` examples
(175 active Matplotlib figures in total), 31 `NO_FIGURES`, 71 `NO_SAMPLES`,
4 `SKIPPED_BIN` and 8 `FAIL`. These counts mix **example-level outcomes**
with package-level `NO_SAMPLES` and one preparation failure; they are not
95 independent plotting tests. See `STATUS.md` for the failure triage and
subsequent focused retest.

The BVAR example `Usage_examples/BVAR_large.inp` loads data via:

```hansl
open examples/Usage_examples/CONSUMPTIONLOGALL.gdt
```

That relative path is resolved against libgretl's `$workdir`, not necessarily
Python's `os.getcwd()`. The installed package root contains the data file.
Changing only Python's cwd still caused `FileNotFoundError`; synchronizing
`$workdir` via `set workdir` resolved it. The focused regression command is:

```bash
python probe_package_batch.py BVAR \
    --samples Usage_examples/BVAR_large.inp \
    --approved --continue-on-error --skip-bin
```

The verified result is `NO_FIGURES` with `FAIL=0` (the script finishes, but
does not leave an active Matplotlib figure). A complete candidate rerun **after**
this change has not yet been reported; do not overwrite the full-run baseline
with the focused result.

Reports are placed in `package_plot_results/batch/`:

- `batch_results.csv`: package, sample, status, reason and log path.
- `logs/`: full combined stdout/stderr per sample, including native diagnostics.
- `images/`: rendered PNGs for samples that return active Matplotlib figures.

Statuses: `PASS` means at least one figure was rendered and no error was detected;
`FAIL` means a Python error, a native `gretl_spawn`/`gptranslate` diagnostic,
missing result marker, or a timeout; `NO_FIGURES` means the sample finished
without a detected error but no active figure was found; `NO_SAMPLES` means a
package (addon or contributed) has no installed `.inp` examples to run;
`SKIPPED_BIN` means an example was deliberately not executed because it
uses a native `.bin` database. Neither `NO_FIGURES` nor `NO_SAMPLES` proves
that a package lacks plot functionality. The batch returns a nonzero exit
status if any failure is recorded. A `FAIL` must be classified from its log;
it does not automatically indicate a `gptranslate` defect.

**Why native-output detection matters:** `gretl.run_script()` may print
`gretl_spawn: status = 512` or `Error in call_gridplot` without propagating a
Python exception. The updated `probe_package_sample.py` supervises its own
worker process and reports such a failure correctly. In batch mode, the runner
executes workers directly and detects the native error in their combined output.
This detects the known error formats, but cannot guarantee detection of every
possible libgretl error message. A green batch result is not yet a visual
comparison against gnuplot.

```bash
# Standalone probes now also detect printed native errors and exit nonzero.
python probe_package_sample.py PairPlot --sample iris1.inp --approved

# List recursive examples and run a reviewed addon sample.
python probe_package_sample.py SVAR --list-samples
python probe_package_sample.py SVAR --source addon \
    --sample SetRestrictions/spaghetti_plot.inp --approved

# A unique basename is also accepted; ambiguous names require the full relative path.
python probe_package_sample.py SVAR --sample BlQuah --approved

# Run the scanner and the offline unit tests.
python -m unittest -v test_package_plot_audit test_probe_package_sample test_probe_package_batch
```

### Nested example discovery and selection

Examples are discovered recursively (`examples/**/*.inp`); displayed paths are
relative to `examples/`. Names and case are preserved. `--sample BlQuah` and
`--samples BlQuah` work only if exactly one installed file has that basename.
For duplicates, pass `--sample Traditional/BlQuah.inp` or the corresponding
`--samples` value. Relative path traversal, absolute paths and symlinks that
resolve outside `examples/` are rejected. Missing examples for addons result
in `NO_SAMPLES` in batch mode; they are not passed to `run_sample()` by default.
This changes example selection only; addon loading and local-cache installation
remain unchanged.


### Temporarily skip examples using binary gretl databases

Until `gretl4py` supports loading native `.bin` databases, use **`--skip-bin`**
for batch runs. The probe checks each discovered `.inp` before execution and
skips examples with a literal `open NAME.bin` command (including quoted names).
The command is not run and is recorded as `SKIPPED_BIN`, not `FAIL`, in both
`batch_results.csv` and its diagnostic log.

```bash
# Continue the wider audit without the known unsupported .bin input.
python probe_package_batch.py --candidates --contributed-only \
    --approved --continue-on-error --skip-bin

# Focused cffilt run.
python probe_package_batch.py cffilt --approved --skip-bin --continue-on-error

# Once gretl4py supports .bin: drop --skip-bin to test these cases again.
python probe_package_batch.py cffilt --samples replication.inp --approved
```

This filter deliberately recognizes **literal, direct** `open *.bin` statements.
It does not expand hansl variables, scan dynamically included code, or recognize
database paths computed at runtime. Such failures can still occur and should be
investigated normally. All previous behavior is preserved without `--skip-bin`.


## Continue candidate batches with `--offset`

The optional `--offset N` skips the first N packages in the selected,
deduplicated candidate list. It is applied before `--limit`, and is compatible
with `--skip-bin` and `--continue-on-error`. Example: 

```bash
python probe_package_batch.py --candidates --contributed-only \
    --offset 5 --limit 5 --approved --skip-bin --continue-on-error
```

`--offset` defaults to 0. `--offset` must not be negative. Ordering follows
the scanner's current `plot_packages.csv` and may change if the catalog
is rescanned or sorted differently.
