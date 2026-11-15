# Status at 2026-11-15

## Executive summary

The three stages of the maintained `priority5` plotting audit are complete:

| Verification | Result | Evidence / scope |
| --- | ---: | --- |
| `gptranslate` Meson suite | **83/83 PASS** | User's complete Meson result, 15 Nov 2026 |
| `priority5` integration suite | **17/17 PASS** | Complete `--custom priority5` batch, 15 Nov 2026 |
| Packages covered by `priority5` | **15** | Distinct public gretl function packages |
| Matplotlib figures | **19** | Figures produced in the complete integration run |
| Audit runner offline unit tests | **54/54 PASS** | Local `unittest discover`, verified 15 Nov 2026 |

All **17 cases passed together** in one batch run, after the earlier
case-by-case fixes. This result verifies end-to-end execution through
gretl4py/libgretl, the `gp-executor`/`gptranslate` translation path and
Matplotlib figure creation. It does not prove visual equivalence to gnuplot or
exhaustive coverage of the 15 packages.

The Meson total of 83 covers the `gptranslate` test suite; the 17 maintained
package cases are a **separate** audit suite and must not be added to the Meson
test count.

## Maintained integration suite: all three stages complete

| Stage | Cases | Distinct packages introduced | Figures | Result |
| --- | --- | ---: | ---: | --- |
| 1 | `01`–`06` | 5 | 7 | 6/6 PASS |
| 2 | `07`–`12` | 5 | 6 | 6/6 PASS |
| 3 | `13`–`17` | 5 | 6 | 5/5 PASS |
| **Total** | **17** | **15** | **19** | **17/17 PASS** |

**Stage 1:** `TVCplots`, `color_xy`, `gnuplot_piechart`, `candlesticks`
(two variants), `SankeyPlot`.

**Stage 2:** `bandplot`, `kddensity` (contour and heatmap), `season_plot`,
`MDS`, `corresp`.

**Stage 3:** `coefcomp`, `roc`, `fdensity`, `ARMA_IRF`, `LocalProjection`.

Final command and reported result:

```bash
python probe_package_batch.py --custom priority5 \
    --approved --continue-on-error
# [SUMMARY] PASS=17, NO_FIGURES=0, NO_SAMPLES=0, SKIPPED_BIN=0, FAIL=0
```

Default report location:
`package_plot_results/batch/custom/priority5/`, containing
`batch_results.csv`, logs and PNG figures. The report from a subsequent
subset run may overwrite the default report; use `--out` for snapshots that
must be retained.

### Translation capabilities exercised by the maintained cases

The integration work uncovered and tested real constructs including:

- Palette-based colouring, negative palettes and map-style XYZ plots.
- Gnuplot circles/arcs, rectangles, filled curves and scalar expressions.
- OHLC candles, volume panels, multiplot and SI axis tick formatting.
- Inline XYZ data, `dgrid3d`, contour drawing, `pm3d` and heatmap rendering.
- `set size ratio` and `set for [...] linetype ... ps ...`.
- Named axis tics in combination with modifiers such as `nomirror`.
- Styled and aligned labels (`font`, `tc rgb`, `left`, `center`, `right`) and
  graph-relative label coordinates.
- Horizontal `xerrorbars` and associated `lc black` style handling.

Translation/parser changes were accompanied by dedicated tests within
`gptranslate`; after the final ROC-label correction, the **complete Meson
suite passed 83/83**. The maintained hansl fixtures were updated separately
when failures were caused by invalid test data or call signatures. No fix
required modifying an upstream package.

The `LocalProjection` integration log also showed an upstream deprecation
warning about legacy `--band` syntax; this was **not** a test failure and
is not evidence of a `gptranslate` regression.

## Scope and inventories

- Initial public contributed catalog: **172 entries** (as recorded when the
  inventory was assembled). The official addon catalog is separate.
- Exploratory scanner: **95 candidate packages** in the 14 November 2026
  batch; selection does not claim coverage of all public packages or plotting
  functions.
- `priority_candidates.csv`: 28 initially selected planning targets, not a
  verified list of all plotting functions.
- Sources: <https://gretl.sourceforge.net/cgi-bin/gretldata.cgi>,
  <https://gretl.sourceforge.net/addons-data/addons.xml>,
  <https://github.com/gretl-project/gretl/tree/master/addons>.

## Historical 95-candidate exploratory batch (14 November 2026)

This snapshot was obtained via:

```bash
python probe_package_batch.py --candidates \
    --approved --continue-on-error --skip-bin
```

| Status | Count | Interpretation |
| --- | ---: | --- |
| `PASS` | 54 | Examples with at least one active Matplotlib figure |
| `NO_FIGURES` | 31 | Examples executed without an active figure |
| `NO_SAMPLES` | 71 | Candidate packages without a discovered installed `.inp` sample |
| `SKIPPED_BIN` | 4 | Examples skipped because they directly opened `.bin` data |
| `FAIL` | 8 | Seven example failures plus one package preparation failure |

The 54 passing examples produced **175 figures**. These categories are
**not additive**: `NO_SAMPLES` counts packages, whereas the others mostly
count examples, plus one package preparation failure. Among the 95 candidates,
71 had no installed examples, 23 had examples discovered, and one
(`random_forest`) failed during package preparation.

This is a *historical baseline*, **not an updated summary after the recent
`gptranslate` fixes**. The complete 95-candidate batch has not been rerun
since the BVAR working-directory fix and the subsequent translation work.
The 17-case maintained suite is a different, more targeted experiment and
must not be merged arithmetically with these 95-candidate outcomes.

### Outstanding exploratory failures (historical triage)

| Package | Failing item | Finding / status |
| --- | --- | --- |
| `BVAR` | `Usage_examples/BVAR_large.inp` | Relative path issue **fixed**, focused retest successful |
| `FLS` | `fls_compare_to_R.inp` | R dependency/integration; not resolved |
| `CUB` | `compare_vs_R.inp` | R dependency/integration; not resolved |
| `MAR` | `MAR_sample.inp` | Refers to absent `MAR_10.inp` |
| `kmeans` | `phoneme.inp` | External R execution/dependency problem |
| `DCC` | `dj.inp` | Refers to absent `DCC.inp` |
| `BACE` | `session.inp` | Runtime-generated file erroneously treated as sample |
| `random_forest` | `<prepare>` | `Rlib_check_` lookup failure in embedded R initialization; unresolved |

These findings should be revisited individually. A package/data/environment
failure does not establish a translator defect.

### BVAR workdir fix: focused validation completed

`BVAR_large.inp` uses the relative path
`open examples/Usage_examples/CONSUMPTIONLOGALL.gdt`. Python's cwd alone was
insufficient; the runner also needs to synchronize libgretl's `$workdir` using
`set workdir`. The focused retest:

```bash
python probe_package_batch.py BVAR \
    --samples Usage_examples/BVAR_large.inp \
    --approved --continue-on-error --skip-bin
```

returned `NO_FIGURES=1, FAIL=0`: the example finished without an error but
did not produce an active figure. It has not changed the historical count of
eight failures, because a complete 95-candidate rerun is pending.

## Next actions

1. **Visual validation:** inspect the 19 PNGs in the maintained suite and,
   where reference output is available, compare layout, series, labels,
   palettes, error-bar geometry and bands against gnuplot. A PASS only verifies
   successful execution and figure creation.
2. **Repository hygiene:** check `git status --short`, `git diff --check` and
   the intended Meson/test fixture files; commit `gptranslate` changes and
   `gretl4py` audit changes **separately**, after review.
3. **Exploratory rerun:** rerun the 95-candidate batch with the new translator
   and publish a new dated baseline without overwriting the 14 November result.
4. **Failure triage:** exclude generated BACE `session.inp` from candidate
   examples and investigate R-dependent failures (`FLS`, `CUB`, `kmeans`,
   `random_forest`), missing sample scripts (`MAR`, `DCC`) and `.bin` support.
5. **Future coverage:** identify further public plotting functions not reached
   by example scripts; add deterministic fixtures and geometry-level
   assertions for selected high-value plots.

## Interpretation and limitations

Static scans are heuristic and do not establish that a plotting path can be
reached via the public API. `NO_SAMPLES` and `NO_FIGURES` do not imply that a
package cannot plot. The maintained 17-case integration suite is a verified
functional smoke test, but neither it nor the 83 Meson tests guarantee
pixel-identical images or exhaustive compatibility with gnuplot.
