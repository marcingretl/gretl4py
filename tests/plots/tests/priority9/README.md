# priority9 — Quantile cross-dependence, PDL forecasts and impulse responses

This maintained integration batch has **8 cases** across **2 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** Heatmaps; forecasts; dynamic responses.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_qcorr_heatmap` | `qcorr` | qcorr: heatmap of cross-quantilograms at lag one. |
| `02_qcorr_normalized` | `qcorr` | qcorr: bootstrap-normalized cross-quantilogram heatmap. |
| `03_qcorr_lagplot` | `qcorr` | qcorr: lag-by-lag correlation plot and bootstrap confidence bounds. |
| `04_qcorr_testplot` | `qcorr` | qcorr: Ljung-Box statistic plot and bootstrap critical values. |
| `05_lagreg_forecast90` | `lagreg` | lagreg: PDL forecast with 90 percent confidence limits. |
| `06_lagreg_forecast95` | `lagreg` | lagreg: longer PDL forecast at 95 percent confidence level. |
| `07_lagreg_irf` | `lagreg` | lagreg: ordinary impulse response of a distributed-lag (PDL) model. |
| `08_lagreg_cumirf` | `lagreg` | lagreg: cumulative impulse response of a distributed-lag (PDL) model. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority9 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority9 \
    --cases 01_qcorr_heatmap \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority9/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
