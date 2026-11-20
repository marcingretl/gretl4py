# priority16 — contributed-package plot probes (exploratory, unrun)

Eight **unrun** independent graph-producing tests for `pcaTools` and `BVAR`.
The verified `priority5`–`priority15` baseline is **96/96 PASS, 115 figures,
44 distinct packages**. Neither new package is included in that maintained
baseline. `BVAR` was seen in the earlier 95-candidate exploratory audit, but
had no maintained plot-only probes.

| Case | Package | Plot path |
| --- | --- | --- |
| `01_pcaTools_scree_std` | pcaTools | Standardized PCA scree plot |
| `02_pcaTools_scree_cov` | pcaTools | Covariance PCA scree plot and font size |
| `03_pcaTools_biplot_basic` | pcaTools | Biplot of the first two components |
| `04_pcaTools_biplot_factor` | pcaTools | Factor-coloured biplot with loading arrows |
| `05_BVAR_fcast_single` | BVAR | Forecast with posterior bands, one series |
| `06_BVAR_fcast_grid` | BVAR | Two forecasts as a gridplot, including posterior mean |
| `07_BVAR_sirf_selected` | BVAR | Selected Cholesky structural IRF |
| `08_BVAR_irf_grid` | BVAR | Reduced-form IRFs, gridplot layout |

## Public API and source verification

The package author's public `pcaTools` README documents:

```hansl
bundle pc = pcaEst(X, _(do_stdize=1, use_vcv=0))
pcaScreeplot(pc)
pc.n_pcs_to_plot = 2
pcaBiplot(pc)
```

`pcaScreeplot` and `pcaBiplot` both return **void**; do not assign their
results. Scree plotting accepts `pc.fontsize`; the biplot accepts
`pc.factor` as a discrete series, `pc.color_arrow`, `pc.pointsize`, and
`pc.n_pcs_to_plot` to limit the number of component pairs.

**Installation caveat:** `pcaTools` may not be on the official function
package server. The author provides its `src/pcaTools.gfn` in the linked
repository, with instructions to install via `pkg install ... --local`.
Check that `gretl.include("pcaTools")` can resolve this package. The package
has a documented dependency on `fsboost`, and biplots may also require
`PairPlot` to be available. The fixtures do not download any dependencies.

The official gretl BVAR manual documents:

```hansl
bundle mod = BVAR_setup(Y, 1, "diff")
bundle post = BVAR_posterior(mod, "fcast", _(iter=399, fcast_h=8, mpi_np=1))
BVAR_fcast_plot(post, Y, _(grid=1))
```

For impulse-response plots, `BVAR_posterior(mod, "irf", ...)` is followed
by `BVAR_irf_plot(post, shocks, responses, options)`. Its `grid` option
controls plot collection, and `irf_type="irf"` requests reduced-form rather
than structural responses. All omitted priors use package defaults. The
manual's prose describes the last IRF argument as a plot-option bundle,
although one printed type signature labels it `bool`; these fixtures follow
the provided example and documented option keys.

Source documentation:
- https://github.com/atecon/pcaTools
- https://gretl.sourceforge.net/current_fnfiles/unzipped/BVAR.pdf

## Preparation, execution and limitations

- Eight isolated hansl scripts use seed `20261118` and synthetic data.
- Each script has a `preload/CASE.inp` file, because some packages need a
  dataset active at `gretl.include()` time; the actual script resets it.
- `BVAR` uses **399 posterior draws** and `mpi_np=1` for smoke testing,
  not for econometric inference. Depending on the package version this
  may expose numerical or API differences. Examine the worker log first.
- All plotting functions are called for effect and never assigned to
  a new variable. This avoids the void-return mistake found in priority14.
- `PASS` means the path executed and generated Matplotlib figures; it
  does not establish that the images are visually identical to gnuplot.

Run a simple scree plot and a forecast before the other six:

```bash
python probe_package_batch.py --custom priority16 \
  --cases 01_pcaTools_scree_std 05_BVAR_fcast_single \
  --approved --continue-on-error
```

Then the remaining cases:

```bash
python probe_package_batch.py --custom priority16 \
  --cases 02_pcaTools_scree_cov 03_pcaTools_biplot_basic \
          04_pcaTools_biplot_factor 06_BVAR_fcast_grid \
          07_BVAR_sirf_selected 08_BVAR_irf_grid \
  --approved --continue-on-error
```

After correcting any fixture or environment failures, run the complete
suite:

```bash
python probe_package_batch.py --custom priority16 \
  --approved --continue-on-error
```

Reports: `package_plot_results/batch/custom/priority16/`.
Partial runs can overwrite the latest batch_results.csv.
