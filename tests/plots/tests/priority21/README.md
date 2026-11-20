# priority21 — maps and structural-break plot paths (unverified)

Eight reviewed **but not yet executed** integration fixtures. One **new** package,
`geoplot`, exercises gretl's built-in map-rendering path (implemented in C,
with supporting functions and GeoJSON resources supplied by the official addon).
The other four cases extend the existing `ZivotAndrews` package coverage from
`priority7`, exercising its **package-native `ZA_plot()`** function.

## Coverage

| Fixture | Package | What is plotted |
| --- | --- | --- |
| `01_geoplot_us_outlines` | geoplot | Polygon outlines without a payload |
| `02_geoplot_us_blue_palette` | geoplot | Blue-scale choropleth for synthetic values |
| `03_geoplot_us_mainland` | geoplot | Contiguous US states via `postal` filtering |
| `04_geoplot_us_lambert` | geoplot | Transformed US geometry and axis ticks (EPSG2163) |
| `05_ZA_model_A_default` | ZivotAndrews | Model A, default t-stat lag selector |
| `06_ZA_model_B_fixed` | ZivotAndrews | Model B, seven fixed lags |
| `07_ZA_model_C_bic` | ZivotAndrews | Model C, BIC lag selection |
| `08_ZA_model_A_aic` | ZivotAndrews | Model A, AIC lag selection |

## APIs, data requirements and provenance

- `geoplot(payload, options)` is a built-in gretl function, with GIS data
  supplied by the **official `geoplot` addon**. It can map a loaded
  GeoJSON dataset; its options include `palette`, `border`, `title`, `tics`,
  and `projection`. The `us-states.geojson` source is opened from the addon:
  `open us-states.geojson --frompkg=geoplot`.
- **Caveat:** The runner's package loader must be able to load `geoplot` as
  an addon. If it reports a package-load failure, do not attribute it to
  the translation layer; investigate addon availability / package exposure.
- The source's documented `geoplot()` display behavior should be tested
  without setting `plotfile`, which can merely write a `.plt` artifact.
  If no Matplotlib figure is registered, inspect both the generated file and
  the `gp-executor` dispatch path first.
- `ZivotAndrews`: `ZA_test(series, model_type, opts)` returns a bundle;
  `ZA_plot(bundle)` draws the full breakpoint statistic path and critical
  values. `model_type` is 1 (A), 2 (B), or 3 (C), and documented methods
  include `fixed`, `aic`, `bic`, `hqc` and `tstat`.
- `np.gdt` is the standard gretl Nelson–Plosser dataset. The test uses
  `log(rpcgnp)`, exactly as in the published ZivotAndrews example.
- The `geoplot` numerical payload is synthetic, seeded for reproducibility;
  it is not an empirical US indicator. Tests target visualization only.

Authoritative reference pages checked on 20 Nov 2026:
- https://fossies.org/linux/gretl/addons/geoplot/doc/geoplot.tex
- https://fossies.org/linux/gretl/addons/geoplot/examples/founders.inp
- https://gretl.sourceforge.net/gretl-help/funcref.html
- https://gretl.sourceforge.net/current_fnfiles/unzipped/ZivotAndrews.pdf

## Limits and commands

`PASS` means the script finished with at least one active Matplotlib figure;
it does **not** establish pixel equality with gnuplot or visual correctness.

```bash
python probe_package_batch.py --custom priority21 \
  --cases 01_geoplot_us_outlines 05_ZA_model_A_default \
  --approved --continue-on-error

python probe_package_batch.py --custom priority21 \
  --approved --continue-on-error
```

Both `geoplot` and `ZivotAndrews` must be installed with their example data.
Reports: `package_plot_results/batch/custom/priority21/`.
Subsets overwrite `batch_results.csv` in this output directory; save a
separate copy if you need to compare runs.
