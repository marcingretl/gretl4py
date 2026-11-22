# priority5 — Foundational graphics, style commands, custom shapes, density, IRFs and bands

This maintained integration batch has **17 cases** across **15 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** Fundamental plot types; statistical diagnostics; graphics syntax.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_TVCplots` | `TVCplots` | TVCplots v3.0: native confidence bands with opacity and fill style. |
| `02_color_xy` | `color_xy` | color_xy v2.1: inline $DATA, mapped splot and palette-coloured lines/points. |
| `03_gnuplot_piechart` | `gnuplot_piechart` | gnuplot_piechart v1.2: sectors made via 'set obj circle arc', screen labels. |
| `04_candlesticks_basic` | `candlesticks` | candlesticks v1.1: OHLC candles without the optional volume panel. |
| `05_candlesticks_volume` | `candlesticks` | candlesticks v1.1: OHLC plus volume exercises two-panel multiplot layout. |
| `06_SankeyPlot` | `SankeyPlot` | SankeyPlot v1.0: gnuplot function definitions, object rectangles, filledcurves. |
| `07_bandplot` | `bandplot` | bandplot: confidence band for an OLS slope, drawn with a filled band. |
| `08_kddensity_contour` | `kddensity` | kddensity: bivariate kernel density with 2D contour lines. |
| `09_kddensity_heatmap` | `kddensity` | kddensity: the same input as the contour case, but pm3d heatmap output. |
| `10_season_plot` | `season_plot` | season_plot: use a complete synthetic quarterly series spanning 20 years. |
| `11_MDS` | `MDS` | MDS: a symmetric Euclidean distance matrix for four square vertices. |
| `12_corresp` | `corresp` | corresp: correspondence analysis and row/column biplot labels. |
| `13_coefcomp` | `coefcomp` | coefcomp: compare three deterministic coefficient estimates and intervals. |
| `14_roc` | `roc` | roc: a non-degenerate binary outcome and deterministic probability scores. |
| `15_fdensity` | `fdensity` | fdensity: factorized kernel densities for two groups with ample observations. |
| `16_ARMA_IRF` | `ARMA_IRF` | ARMA_IRF: fit a simple univariate AR(1) and plot its Wold representation. |
| `17_LocalProjection` | `LocalProjection` | LocalProjection: linear local-projection IRF with a small synthetic system. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority5 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority5 \
    --cases 01_TVCplots \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority5/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
