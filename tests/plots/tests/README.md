# Package plotting integration suites — thematic index

The maintained suite IDs `priority5`–`priority32` are **stable runner identifiers**, not a taxonomy. Do not rename or move suite directories, `manifest.csv`, `samples/` or `preload/` without changing and retesting `probe_package_batch.py` and its offline tests. This index groups them **logically**, preserving all working execution paths.

**Snapshot (22 November 2026):** 28 suites, **250 case fixtures** counted from the manifests, **250/250 PASS** and **311 Matplotlib figures** in the completed audit. The 311 figure count is a reported execution result, not a property of the manifests. Figure creation alone does not prove visual or numerical correctness.

## Run

From `tests/plots/`:

```bash
python probe_package_batch.py --custom priority32 --approved --continue-on-error
python probe_package_batch.py --custom priority32 \
    --cases 01_Imputer_missing_backfill --approved --continue-on-error
```

## All maintained suites

| Suite | Cases | Packages | Main themes |
| --- | ---: | --- | --- |
| [`priority5`](priority5/README.md) | 17 | `TVCplots`, `color_xy`, `gnuplot_piechart`, `candlesticks`, `SankeyPlot`, `bandplot`, `kddensity`, `season_plot`, `MDS`, `corresp`, `coefcomp`, `roc`, `fdensity`, `ARMA_IRF`, `LocalProjection` | Fundamental plot types; statistical diagnostics; graphics syntax |
| [`priority6`](priority6/README.md) | 7 | `Brown`, `Holt`, `BreitungCandelonTest`, `naiveFC`, `random_forest` | Forecasts; time series; R-backed graphics |
| [`priority7`](priority7/README.md) | 8 | `Winters`, `tsdecomp`, `hamilcycle`, `ZivotAndrews`, `fsboost` | Forecasts; decomposition; structural breaks |
| [`priority8`](priority8/README.md) | 8 | `VARrec`, `johansenrec`, `BEKK`, `MSVAR`, `PanelTools` | VAR/VECM; volatility; panels |
| [`priority9`](priority9/README.md) | 8 | `qcorr`, `lagreg` | Heatmaps; forecasts; dynamic responses |
| [`priority10`](priority10/README.md) | 8 | `DynMultCalc`, `ghosts` | Frequency domain; dynamic responses; multiplot |
| [`priority11`](priority11/README.md) | 8 | `FEP`, `PTconf` | Loss functions; uncertainty bands |
| [`priority12`](priority12/README.md) | 8 | `PairPlot`, `ketvals` | Scatter matrices; interval estimation |
| [`priority13`](priority13/README.md) | 8 | `QMLSV`, `TVC` | Volatility; time-varying parameters |
| [`priority14`](priority14/README.md) | 8 | `kmeans`, `GHegy` | Clustering; seasonality; unit roots |
| [`priority15`](priority15/README.md) | 8 | `BayTool`, `ParMA` | Bayesian inference; MCMC |
| [`priority16`](priority16/README.md) | 8 | `pcaTools`, `BVAR` | Dimensionality reduction; VAR forecasts |
| [`priority17`](priority17/README.md) | 8 | `gig`, `CUB` | Volatility; categorical distributions |
| [`priority18`](priority18/README.md) | 8 | `StrucTiSM`, `FLS` | Time series; coefficient paths |
| [`priority19`](priority19/README.md) | 8 | `MAR`, `Holt` | Nonlinear VAR; forecasts |
| [`priority20`](priority20/README.md) | 8 | `ADMBP`, `BVAR` | VAR/IRF; uncertainty bands |
| [`priority21`](priority21/README.md) | 8 | `geoplot`, `ZivotAndrews` | Geospatial; structural breaks |
| [`priority22`](priority22/README.md) | 8 | `TenTS`, `basis_functions` | Tensor plots; spline bases |
| [`priority23`](priority23/README.md) | 8 | `ETS`, `BACE` | Forecasts; Bayesian inference |
| [`priority24`](priority24/README.md) | 8 | `DFM`, `DCC` | PCA; factor models; heatmaps |
| [`priority25`](priority25/README.md) | 8 | `GFE`, `dcm` | Panel effects; discrete choices |
| [`priority26`](priority26/README.md) | 8 | `cffilt`, `Canova_Hansen` | Spectral plots; seasonal adjustment |
| [`priority27`](priority27/README.md) | 8 | `thresh_infer`, `Threshold_Panel` | Parameter stability; threshold models |
| [`priority28`](priority28/README.md) | 8 | `ARprobit`, `staticfactor` | Binary time-series models; PCA |
| [`priority29`](priority29/README.md) | 20 | `gregory_hansen`, `var_chow`, `knn`, `tsfcst`, `gen_hurst` | Structural breaks; machine learning; forecasts |
| [`priority30`](priority30/README.md) | 20 | `delayspectral`, `VSG_test`, `uniFCextensions`, `carr`, `TsCrossValidation` | Spectral analysis; forecast bands; cross-validation |
| [`priority31`](priority31/README.md) | 6 | `BMA`, `nic_gig`, `wstats` | Heatmaps; histograms; analytic functions; boxes |
| [`priority32`](priority32/README.md) | 4 | `Imputer`, `SETAR`, `RollingStats` | Missing data; time alignment; threshold LR |

## Topic-oriented navigation

One suite may appear under more than one topic; the case-level truth is always the original manifest and the per-suite README.

| Topic | Relevant suites |
| --- | --- |
| Time-series lines, decomposition, rolling and seasonal plots | [`priority5`](priority5/README.md), [`priority6`](priority6/README.md), [`priority7`](priority7/README.md), [`priority18`](priority18/README.md), [`priority19`](priority19/README.md), [`priority22`](priority22/README.md), [`priority26`](priority26/README.md), [`priority32`](priority32/README.md) |
| Forecasting, forecast bands and interval graphics | [`priority6`](priority6/README.md), [`priority7`](priority7/README.md), [`priority9`](priority9/README.md), [`priority10`](priority10/README.md), [`priority19`](priority19/README.md), [`priority23`](priority23/README.md), [`priority29`](priority29/README.md), [`priority30`](priority30/README.md), [`priority31`](priority31/README.md) |
| VAR/VECM, IRFs and dynamic responses | [`priority5`](priority5/README.md), [`priority8`](priority8/README.md), [`priority9`](priority9/README.md), [`priority10`](priority10/README.md), [`priority16`](priority16/README.md), [`priority19`](priority19/README.md), [`priority20`](priority20/README.md), [`priority29`](priority29/README.md) |
| Structural breaks, stability, threshold tests | [`priority7`](priority7/README.md), [`priority8`](priority8/README.md), [`priority14`](priority14/README.md), [`priority21`](priority21/README.md), [`priority27`](priority27/README.md), [`priority29`](priority29/README.md), [`priority30`](priority30/README.md), [`priority32`](priority32/README.md) |
| Density, distributions, histograms and uncertainty | [`priority5`](priority5/README.md), [`priority11`](priority11/README.md), [`priority13`](priority13/README.md), [`priority15`](priority15/README.md), [`priority17`](priority17/README.md), [`priority23`](priority23/README.md), [`priority25`](priority25/README.md), [`priority30`](priority30/README.md), [`priority31`](priority31/README.md) |
| Heatmaps, palettes, contours and geographic maps | [`priority5`](priority5/README.md), [`priority9`](priority9/README.md), [`priority17`](priority17/README.md), [`priority21`](priority21/README.md), [`priority24`](priority24/README.md), [`priority31`](priority31/README.md) |
| Panel, multi-figure and layout composition | [`priority5`](priority5/README.md), [`priority7`](priority7/README.md), [`priority8`](priority8/README.md), [`priority10`](priority10/README.md), [`priority12`](priority12/README.md), [`priority16`](priority16/README.md), [`priority18`](priority18/README.md), [`priority22`](priority22/README.md), [`priority24`](priority24/README.md) |
| Dimensionality reduction, clustering and factor models | [`priority5`](priority5/README.md), [`priority12`](priority12/README.md), [`priority14`](priority14/README.md), [`priority16`](priority16/README.md), [`priority24`](priority24/README.md), [`priority28`](priority28/README.md) |
| Bayesian simulation, chains and model averaging | [`priority15`](priority15/README.md), [`priority16`](priority16/README.md), [`priority23`](priority23/README.md), [`priority31`](priority31/README.md) |
| Nonlinear functions, gnuplot constructs and custom graphics | [`priority5`](priority5/README.md), [`priority6`](priority6/README.md), [`priority11`](priority11/README.md), [`priority17`](priority17/README.md), [`priority31`](priority31/README.md) |
| Missing data and observation/date alignment | [`priority28`](priority28/README.md), [`priority29`](priority29/README.md), [`priority31`](priority31/README.md), [`priority32`](priority32/README.md) |

## Layout and data retention

- `manifest.csv`: stable case ID → package → relative `samples/*.inp` path. This is the authoritative executable registry.
- `samples/`: reviewed native hansl input. `preload/`: package-specific initialization; `data/`: deliberately bundled fixture inputs.
- `README.md` in each `priorityNN`: reason for exercising that path, dependencies/limitations and command examples.
- `tests/plots/package_plot_results/`: generated CSV reports, logs and PNG images, not source fixtures. A subset run may overwrite the latest default batch report; use `--out` if retaining evidence.
- Timestamped `.plt` files in `priority31` and `gpttmp01.plt` artifacts in other priorities are generated by gretl execution. Check provenance before treating one as a canonical fixture.
- `tests/plots/audit/contributed/` is a local third-party package cache used by the audit, distinct from the authored fixtures; it should not be mistaken for code developed within gretl4py.

The large [`plots/README.md`](../README.md) describes the scanner and runner; [`STATUS.md`](../STATUS.md) keeps historical audit evidence. Neither should be used as a substitute for a suite manifest.
