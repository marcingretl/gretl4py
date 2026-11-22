# priority8 — Recursive VAR and Johansen diagnostics, volatility and panel quantiles

This maintained integration batch has **8 cases** across **5 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** VAR/VECM; volatility; panels.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_VARrec_forward` | `VARrec` | VARrec: forward recursive VAR coefficient estimation. |
| `02_VARrec_backward` | `VARrec` | VARrec: backward recursive VAR coefficient estimation. |
| `03_johansenrec_eigen` | `johansenrec` | johansenrec: recursively estimated eigenvalue stability charts. |
| `04_johansenrec_beta` | `johansenrec` | johansenrec: recursive beta-constancy statistic and critical value plot. |
| `05_BEKK_combo` | `BEKK` | BEKK: public conditional variance/covariance/correlation gridplot. |
| `06_MSVAR_regirfs` | `MSVAR` | MSVAR: public multi-panel gridplot of regime impulse-response matrices. |
| `07_PanelTools_quantiles` | `PanelTools` | PanelTools: default cross-sectional quantiles through the panel time axis. |
| `08_PanelTools_xquantiles` | `PanelTools` | PanelTools: fractional cross-section quantiles and custom plot styling. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority8 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority8 \
    --cases 01_VARrec_forward \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority8/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
