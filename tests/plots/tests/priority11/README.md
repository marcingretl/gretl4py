# priority11 — Asymmetric loss-function curves and permanent-transitory confidence bands

This maintained integration batch has **8 cases** across **2 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** Loss functions; uncertainty bands.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_FEP_linlin` | `FEP` | FEP: single asymmetric lin-lin loss and its confidence interval. |
| `02_FEP_quadquad` | `FEP` | FEP: single asymmetric quadratic loss and its confidence interval. |
| `03_FEP_compare_linlin` | `FEP` | FEP: two lin-lin loss functions and their confidence intervals. |
| `04_FEP_compare_quadquad` | `FEP` | FEP: compare two asymmetric quad-quad loss curves. |
| `05_PTconf_GG_all` | `PTconf` | PTconf: GG transitory components with asymptotic confidence bands. |
| `06_PTconf_SWP_all` | `PTconf` | PTconf: SWP transitory components with asymptotic confidence bands. |
| `07_PTconf_GG_first` | `PTconf` | PTconf: select only the first GG transitory component. |
| `08_PTconf_SWP_second` | `PTconf` | PTconf: select only the second SWP transitory component. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority11 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority11 \
    --cases 01_FEP_linlin \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority11/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
