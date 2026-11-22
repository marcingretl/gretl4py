# priority7 — Seasonal plots, time-series decomposition, structural breaks and coefficient paths

This maintained integration batch has **8 cases** across **5 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** Forecasts; decomposition; structural breaks.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_Winters_forecast` | `Winters` | Winters: additive seasonal smoothing with an in-sample forecast holdout. |
| `02_Winters_components` | `Winters` | Winters: visualize level, trend, seasonal and irregular components. |
| `03_tsdecomp_vertical` | `tsdecomp` | tsdecomp: classical additive decomposition in a three-row gridplot. |
| `04_tsdecomp_horizontal` | `tsdecomp` | tsdecomp: the same decomposition in a three-column gridplot. |
| `05_hamilcycle` | `hamilcycle` | hamilcycle: quarterly Hamilton-filter cycle over a synthetic series. |
| `06_ZivotAndrews` | `ZivotAndrews` | ZivotAndrews: plot the rolling break-location test statistic and critical values. |
| `07_fsboost_rho` | `fsboost` | fsboost: draw residual-correlation paths over forward-stagewise iterations. |
| `08_fsboost_coefficients` | `fsboost` | fsboost: plot coefficient paths for the selected forward-stagewise predictors. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority7 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority7 \
    --cases 01_Winters_forecast \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority7/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
