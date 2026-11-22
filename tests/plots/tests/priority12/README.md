# priority12 — Pairwise plotting layouts and IV/OLS parameter interval regions

This maintained integration batch has **8 cases** across **2 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** Scatter matrices; interval estimation.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_PairPlot_triangle` | `PairPlot` | PairPlot triangle — PairPlot: independent synthetic cross-sectional data for an isolated plot. |
| `02_PairPlot_matrix_groups` | `PairPlot` | PairPlot matrix groups — PairPlot: independent synthetic cross-sectional data for an isolated plot. |
| `03_PairPlot_row_mean` | `PairPlot` | PairPlot row mean — PairPlot: independent synthetic cross-sectional data for an isolated plot. |
| `04_PairPlot_column_median` | `PairPlot` | PairPlot column median — PairPlot: independent synthetic cross-sectional data for an isolated plot. |
| `05_ketvals_OLS_slope95` | `ketvals` | ketvals OLS slope95 — ketvals: small synthetic model with two relevant external instruments. |
| `06_ketvals_OLS_intercept90` | `ketvals` | ketvals OLS intercept90 — ketvals: small synthetic model with two relevant external instruments. |
| `07_ketvals_IV_slope95` | `ketvals` | ketvals IV slope95 — ketvals: small synthetic model with two relevant external instruments. |
| `08_ketvals_IV_intercept90` | `ketvals` | ketvals IV intercept90 — ketvals: small synthetic model with two relevant external instruments. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority12 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority12 \
    --cases 01_PairPlot_triangle \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority12/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
