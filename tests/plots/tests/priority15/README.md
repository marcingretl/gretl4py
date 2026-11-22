# priority15 — Bayesian MCMC posterior density and trace plots

This maintained integration batch has **8 cases** across **2 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** Bayesian inference; MCMC.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_BayTool_density_x1` | `BayTool` | BayTool density x1 — BayTool: reproducible, low-dimensional synthetic Gaussian regression. |
| `02_BayTool_density_x2_epanech` | `BayTool` | BayTool density x2 epanech — BayTool: reproducible, low-dimensional synthetic Gaussian regression. |
| `03_BayTool_trace_single` | `BayTool` | BayTool trace single — BayTool: reproducible, low-dimensional synthetic Gaussian regression. |
| `04_BayTool_trace_pair` | `BayTool` | BayTool trace pair — BayTool: reproducible, low-dimensional synthetic Gaussian regression. |
| `05_ParMA_marginal_x1` | `ParMA` | ParMA marginal x1 — ParMA: synthetic probit with two strong predictors and one nuisance covariate. |
| `06_ParMA_marginal_grid` | `ParMA` | ParMA marginal grid — ParMA: synthetic probit with two strong predictors and one nuisance covariate. |
| `07_ParMA_trace_x1` | `ParMA` | ParMA trace x1 — ParMA: synthetic probit with two strong predictors and one nuisance covariate. |
| `08_ParMA_trace_x2` | `ParMA` | ParMA trace x2 — ParMA: synthetic probit with two strong predictors and one nuisance covariate. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority15 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority15 \
    --cases 01_BayTool_density_x1 \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority15/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
