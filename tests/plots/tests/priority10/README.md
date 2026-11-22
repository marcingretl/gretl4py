# priority10 — Dynamic multipliers and frequency-domain spectral charts

This maintained integration batch has **8 cases** across **2 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** Frequency domain; dynamic responses; multiplot.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_DynMultCalc_dynamic95` | `DynMultCalc` | DynMultCalc: dynamic multipliers of x in a fitted distributed-lag model. |
| `02_DynMultCalc_cumulative95` | `DynMultCalc` | DynMultCalc: cumulative dynamic multipliers of x. |
| `03_DynMultCalc_MA90` | `DynMultCalc` | DynMultCalc: MA representation of the estimated autoregressive polynomial. |
| `04_DynMultCalc_dynamic99` | `DynMultCalc` | DynMultCalc: ordinary multiplier plot with a nondefault 99% confidence band. |
| `05_ghosts_spectrum` | `ghosts` | ghosts: log-scaled power spectrum of a synthetic seasonal signal. |
| `06_ghosts_coherence` | `ghosts` | ghosts: squared spectral coherence of two related seasonal signals. |
| `07_ghosts_phase` | `ghosts` | ghosts: cross-spectrum phase plot for two related seasonal signals. |
| `08_ghosts_comboplot` | `ghosts` | ghosts: public four-pane spectrum/coherence/phase gridplot. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority10 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority10 \
    --cases 01_DynMultCalc_dynamic95 \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority10/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
