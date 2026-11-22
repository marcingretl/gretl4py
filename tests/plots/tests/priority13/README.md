# priority13 — Stochastic-volatility diagnostics and time-varying coefficients

This maintained integration batch has **8 cases** across **2 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** Volatility; time-varying parameters.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_QMLSV_standard_volatility` | `QMLSV` | QMLSV standard volatility — QMLSV: use the canonical Bollerslev-Ghysels returns supplied with gretl. |
| `02_QMLSV_standard_density` | `QMLSV` | QMLSV standard density — QMLSV: use the canonical Bollerslev-Ghysels returns supplied with gretl. |
| `03_QMLSV_standard_both` | `QMLSV` | QMLSV standard both — QMLSV: use the canonical Bollerslev-Ghysels returns supplied with gretl. |
| `04_QMLSV_HS_volatility` | `QMLSV` | QMLSV HS volatility — QMLSV: use the canonical Bollerslev-Ghysels returns supplied with gretl. |
| `05_TVC_both_varying` | `TVC` | TVC both varying — TVC: reproducible synthetic quarterly regression with gradually changing coefficients. |
| `06_TVC_slope_varying` | `TVC` | TVC slope varying — TVC: reproducible synthetic quarterly regression with gradually changing coefficients. |
| `07_TVC_intercept_varying` | `TVC` | TVC intercept varying — TVC: reproducible synthetic quarterly regression with gradually changing coefficients. |
| `08_TVC_ml_fixed` | `TVC` | TVC ml fixed — TVC: reproducible synthetic quarterly regression with gradually changing coefficients. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority13 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority13 \
    --cases 01_QMLSV_standard_volatility \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority13/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
