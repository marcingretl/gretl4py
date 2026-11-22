# priority6 — Classical forecast methods, spectral causality and an R-backed model

This maintained integration batch has **7 cases** across **5 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** Forecasts; time series; R-backed graphics.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_Brown_linear` | `Brown` | Brown: synthetic quarterly series, linear triple-exponential smoothing plot. |
| `02_Brown_quadratic` | `Brown` | Brown: quadratic smoothing branch of the same public bundle-plot function. |
| `03_Holt_forecast` | `Holt` | Holt: estimate double exponential smoothing and display fitted/forecast path. |
| `04_Holt_components` | `Holt` | Holt: display level, trend and irregular components (ptype=2). |
| `05_BreitungCandelonTest` | `BreitungCandelonTest` | BreitungCandelonTest: frequency-domain causality curve and critical value. |
| `06_naiveFC` | `naiveFC` | naiveFC: random-walk forecast with a Matplotlib time-series chart. |
| `07_random_forest` | `random_forest` | random_forest: run the R-backed estimator and draw feature importances. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority6 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority6 \
    --cases 01_Brown_linear \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority6/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
