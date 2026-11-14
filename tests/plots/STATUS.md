# Status at 2026-11-14

## Scope and inventories

- Public contributed catalog: 172 entries were listed when the initial
  inventory was prepared. The official addon catalog is separate.
- The exploratory scanner's **current candidate list contained 95 packages**.
  These were all covered by the 14 November 2026 candidate-batch run. This
  is not a claim that all public gretl packages, all public plotting functions,
  or all possible execution paths were tested.
- `priority_candidates.csv` records 28 initially selected targets based on
  names/descriptions; it is a planning file, not the final set of executed tests.
- Public sources: <https://gretl.sourceforge.net/cgi-bin/gretldata.cgi>,
  <https://gretl.sourceforge.net/addons-data/addons.xml>,
  <https://github.com/gretl-project/gretl/tree/master/addons>.

## Completed exploratory integration audit

The following **full-run snapshot** was obtained on 14 November 2026 using:

```bash
python probe_package_batch.py --candidates \
    --approved --continue-on-error --skip-bin
```

| Status | Count | Interpretation |
| --- | ---: | --- |
| `PASS` | 54 | Example executions with at least one active Matplotlib figure |
| `NO_FIGURES` | 31 | Example executions without an active figure |
| `NO_SAMPLES` | 71 | Packages with no installed `.inp` example found |
| `SKIPPED_BIN` | 4 | Examples skipped because they directly open a `.bin` database |
| `FAIL` | 8 | Seven example failures plus one package preparation failure |

**175 Matplotlib figures** were reported by the 54 successful examples. The
categories above have different units: `NO_SAMPLES` is per package, while
`PASS`, `NO_FIGURES`, `SKIPPED_BIN` and example `FAIL` are per example; the
`random_forest` `FAIL` occurred during preparation. The figures count is a
functional smoke-test count, not a pixel-for-pixel comparison against gnuplot.

Of the 95 candidate packages, 71 had no installed examples, 23 had at least
one example discovered, and one (`random_forest`) failed during preparation.
The presence of `NO_SAMPLES` or `NO_FIGURES` **does not establish the absence
of graphing functionality**.

## Failure triage from the full-run snapshot

| Package | Failing item | Current assessment |
| --- | --- | --- |
| BVAR | `Usage_examples/BVAR_large.inp` | Relative data path; **fixed and retested**, see below |
| FLS | `fls_compare_to_R.inp` | R integration/dependency; needs environment-level confirmation |
| CUB | `compare_vs_R.inp` | R integration/dependency; needs environment-level confirmation |
| MAR | `MAR_sample.inp` | Script refers to unavailable `MAR_10.inp` |
| kmeans | `phoneme.inp` | External R execution/dependency issue after translation changes |
| DCC | `dj.inp` | Script refers to unavailable `DCC.inp` |
| BACE | `session.inp` | Generated runtime file incorrectly detected as an example |
| random_forest | `<prepare>` | `Rlib_check_` lookup failure during embedded R setup; cause remains open |

These are triage findings, not proof that every underlying cause is resolved.
No new explicitly reported `unsupported gnuplot command` error arose in the
full-run summary. The failure logs are in
`package_plot_results/batch/logs/`.

### BVAR working-directory regression: resolved

`BVAR_large.inp` contains the relative command
`open examples/Usage_examples/CONSUMPTIONLOGALL.gdt`. The data file exists
under the installed BVAR package root. Merely changing Python's `cwd` did
not fix the `FileNotFoundError`: libgretl also uses its own `$workdir`.
The sample runner now sets `set workdir` to the installed package root when
handling this kind of package-root-relative example, while retaining the
existing selective Python cwd adjustment.

After this change, the **focused** command:

```bash
python probe_package_batch.py BVAR \
    --samples Usage_examples/BVAR_large.inp \
    --approved --continue-on-error --skip-bin
```

completed as `NO_FIGURES=1, FAIL=0`, replacing the earlier failure for this
example. **The full 95-package batch has not yet been rerun after the fix**;
therefore the original snapshot above still reports eight failures. The
focused retest did not produce a figure, but it completed without an error.

## Tests and translator progress

- Latest reported test command:
  `python -m unittest discover -p 'test_*.py' -q`.
- Result: **44 tests passed** (`OK`). The displayed `[FAIL]` messages in
  mocked failure scenarios were expected; the suite itself succeeded.
- These 44 tests were run **before** the final `set workdir` adjustment;
  a post-adjustment full unit-test run remains to be done.
- During the wider `gptranslate` work, support was extended for real-world
  package plots including 3D surfaces, contour/palette behavior, parametric
  plots, grouped BACE histograms, and additional axis/margin commands. This
  document records integration observations, not a full change log for
  `gptranslate`.
- Production-quality reproducible plotting tests covering each public API
  entry point and a visual reference comparison are **not completed**.

## Next actions

1. Rerun the unit tests and then the complete 95-candidate batch after the
   `$workdir` adjustment; publish an updated baseline without mixing snapshots.
2. Exclude generated `session.inp` from BACE's discovered example list (do not
   change the original package examples merely to silence the audit).
3. Investigate R-dependent failures separately, including `Rlib_check_` and
   the possible relevance of gretl4py's `$gretldir`; the latter is a hypothesis,
   **not a confirmed cause**.
4. Revisit `.bin` examples if native database support becomes available.
5. Promote selected functional smoke tests into deterministic plotting
   regression tests and, where possible, compare rendered outputs.

## Interpretation

Static source scanning is a first filter, not proof that plotting code is
reachable. Example-level execution tests are useful integration smoke tests,
not exhaustive public API tests. Each failure should be classified as a
package/data/dependency issue, a gretl4py/libgretl integration issue, a
translator/parser issue, or a backend/rendering mismatch. A green execution
result does not guarantee visual equivalence with gnuplot.
