# priority29 — five contributed package plot audits (source-reviewed)

Twenty plotting fixtures, four each for `gregory_hansen`, `var_chow`, `knn`,
`tsfcst` and `gen_hurst`. Derived from the installed/source-reviewed `.gfn` files
in `priority29_sources.tar.gz`, including the packages' public functions,
parameter signatures and sample scripts. **These fixtures require runtime
execution in the user's gretl4py environment; static verification is not a PASS.**

## Origin and scope

| Package | Source-derived API and data | Intended figures |
| --- | --- | --- |
| `gregory_hansen` | `estimateGH()` and `plotGH()`; packaged `joe96_data.gdt` | Four regime-shift profiles (one per test) |
| `var_chow` | `var_chow()`; upstream Australian VAR example with `IUS PUS e2` | Three native plots per call: break-point, sample-split, forecast |
| `knn` | `knn_fit()` + `knn_plot_score()` / `knn_plot_cvscores()` | Regression/classification CV curves and fold distributions |
| `tsfcst` | `stheta()` + `fcast_plot()`; standard `bjg.gdt` | Point forecast and native uncertainty bands |
| `gen_hurst` | `gen_hurst()`; standard `bjg.gdt` | Two R/S and two DFA native log-log graphs |

### Caveats and deliberately bounded workloads

- Install each **complete package from SourceForge**, not by copying `.gfn`
  alone. `gregory_hansen` requires `joe96_data.gdt` alongside its `.gfn`.
  `knn` also declares hard dependencies `extra` and `CvDataSplitter`; these
  are invoked by cross-validation and matrix `drill()`. If missing, install
  the dependencies via Gretl's package installer before diagnosing graphics.
- `var_chow` produces *three* plots per call, not one. Its `rep` argument
  declares a minimum of 99 bootstrap replications; all four tests use 99 and
  a shorter sample (1976:1–1987:4) for VAR(1); VAR(2) instead uses
  a longer sample (1976:1–1990:4) and a bivariate specification to avoid
  ill-conditioned covariance matrices in bootstrap replication. They may take
  materially longer than the other tests. We retain the real bootstrapped
  estimator and do not substitute artificial graphs.
- `gregory_hansen` cases use the package's original data interval and vary
  documented model specifications or ADF lag-selection criteria.
- `knn` tests use the installed standard `denmark.gdt`, to avoid dependence
  on the example's `credscore.gdt`. Cross-validation uses the package's
  exported functions (no hand-crafted scores); the classification target
  is an above-median binary transformation of the observed `LRY` growth
  series. This avoids the constant k=1 precision scores in the level-based target
  that make the native `boxplot` fail.
- `tsfcst` uses standard `bjg.gdt` rather than the sample's `chomage.gdt`.
  Its `stheta()` output supplies actual forecasts and interval bounds to
  `fcast_plot()`. The point-only case explicitly sets `setype=0` to plot
  just the point forecast from the same returned matrix. Do not synthesize
  substitutes if a plot fails.
- `gen_hurst` uses standard `bjg.gdt` and retains >=128 observations even
  after first differencing. The print-suppression flag remains false so the
  function itself emits the native plot.
- A `PASS` from the probe means a figure appeared, not that its axes,
  confidence bands, or data semantics match Gnuplot. Review every PNG.

## All cases

| Case | Package | Intent |
| --- | --- | --- |
| `01_GH_intercept_aic` | `gregory_hansen` | Gregory-Hansen intercept break, AIC lag selection |
| `02_GH_intercept_trend_bic` | `gregory_hansen` | Break in intercept and trend, BIC selection |
| `03_GH_intercept_slope_ttest` | `gregory_hansen` | Intercept and slope break, downward t lag |
| `04_GH_intercept_fixed_lag` | `gregory_hansen` | Intercept break and fixed ADF lag |
| `05_VARChow_nonrobust_VAR1` | `var_chow` | Nonrobust VAR(1) Chow tests; three p-value plots |
| `06_VARChow_HC_VAR1` | `var_chow` | Heteroskedasticity-robust VAR(1) tests |
| `07_VARChow_nonrobust_VAR2` | `var_chow` | Nonrobust VAR(2) Chow tests |
| `08_VARChow_HC_VAR2` | `var_chow` | HC-robust VAR(2) Chow tests |
| `09_KNN_regression_RMSE_curve` | `knn` | Regression K-fold CV and mean RMSE versus k |
| `10_KNN_regression_RMSE_folds` | `knn` | Regression K-fold CV scores across folds |
| `11_KNN_regression_MAE_curve` | `knn` | Alternative MAE score versus k |
| `12_KNN_classification_PRC_folds` | `knn` | Binary classification precision across folds |
| `13_TSFCST_point_forecast` | `tsfcst` | Theta point forecasts without intervals |
| `14_TSFCST_interval_80` | `tsfcst` | Theta point forecasts with 80% bands |
| `15_TSFCST_interval_95` | `tsfcst` | Theta point forecasts with 95% bands |
| `16_TSFCST_short_horizon` | `tsfcst` | Theta forecasts on shorter horizon |
| `17_Hurst_RS_levels` | `gen_hurst` | Generalized R/S log–log regression on levels |
| `18_Hurst_DFA_levels` | `gen_hurst` | DFA log–log regression on levels |
| `19_Hurst_RS_differences` | `gen_hurst` | R/S plot on differenced series |
| `20_Hurst_DFA_differences` | `gen_hurst` | DFA plot on differenced series |

## Execution

From `gretl-gretl4py/tests/plots`, first run one control per package:

```bash
python probe_package_batch.py --custom priority29 \
    --cases 01_GH_intercept_aic 05_VARChow_nonrobust_VAR1 \
            09_KNN_regression_RMSE_curve 13_TSFCST_point_forecast \
            17_Hurst_RS_levels \
    --approved --continue-on-error
```

Then, if the controls pass, run the complete reviewed batch:

```bash
python probe_package_batch.py --custom priority29 \
    --approved --continue-on-error
```

The probe uses matching `preload/` files, then runs the reviewed
`samples/` scripts. If a package plot or data lookup fails, inspect the
full diagnostic log and installed resources, and distinguish a package
issue from a translator parser issue. Do not change package code or
replace native plots just to obtain a PASS.
