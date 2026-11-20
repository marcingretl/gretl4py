# priority19 — native package plotting APIs (experimental)

Eight reviewed, **not yet executed** fixtures. The suite tests **package-native
plotting entry points** rather than manually plotting returned model results.

- `MAR` (four cases) is **new** to the maintained 50-package set. It is also
  the example previously marked `SKIPPED_INCLUDE` in the broad candidate audit
  because `MAR_sample.inp` refers to missing `MAR_10.inp`. These tests **do not
  execute that example script**; instead they call documented public functions
  directly, using `Datavec.gdt --frompkg=MAR` from the package distribution.
- `Holt` (four cases) was already represented in `priority6`. These cases
  provide **additional coverage of the current Holt 2.0 API**, especially
  custom confidence intervals and pseudo-out-of-sample forecasts. Do not
  increment the distinct-package total for `Holt`.

## Verified documented API (not yet runtime-verified)

**MAR version 1.0**:

```hansl
scalar MAR(lists In, scalar lag, int intercept, int est_method, scalar maxiter, bundle *bdl)
matrix MAR_IRFs(scalar s_i, scalar s_j, scalar horizon, int verb, bundle *x)
```

The public MAR example builds `lists X=defarray(S1,S2,S3,S4)` from `V1`–`V8`
and calls `MAR(X,1,1,1,maxiter,&bdl)` followed by
`MAR_IRFs(1,2,10,,&bdl)` (omitted `verb` requests the default TABLE & PLOT
mode). Both LSE (1) and MLE (2), with an intercept (1), are exercised. The
first two shock indices are **1-based**. The package documentation describes
`Datavec.gdt --frompkg=MAR` and the matrix-valued 2 x 4 data layout.

**Holt version 2.0**, released 3 October 2026:

```hansl
bundle Holt(const series y, const bundle specs[null])
void Holt_plot(bundle *b, int ptype, matrix ci_levels[null])
```

The `ptype` values are 1 (actual and forecasts, optionally with CIs) and 2
(level/trend/irregular components). For forecasts `ci_levels` can be a two-
element vector containing distinct values between 50 and 99 (percent).
`Holt(pop, _(ntest=4))` gives a pseudo-out-of-sample test; `dataset addobs 10`
with the bundled `auspop.gdt` yields out-of-sample forecasts.

Official sources checked 20 Nov 2026:

- https://gretl.sourceforge.net/current_fnfiles/unzipped/MAR.pdf
- https://gretl.sourceforge.net/current_fnfiles/unzipped/Holt.pdf

## Cases

| Case | Package | Graph path |
| --- | --- | --- |
| `01_MAR_LSE_GIRF_1_2` | MAR | LSE generalized IRF for shock (1,2), H=8 |
| `02_MAR_LSE_GIRF_2_1` | MAR | LSE generalized IRF for shock (2,1), H=12 |
| `03_MAR_MLE_GIRF_1_1` | MAR | MLE generalized IRF for shock (1,1), H=8 |
| `04_MAR_MLE_GIRF_2_4` | MAR | MLE generalized IRF for shock (2,4), H=10 |
| `05_Holt_OOS_default` | Holt | Actual vs out-of-sample forecasts and default CI |
| `06_Holt_components` | Holt | Tripartite component plot |
| `07_Holt_CI_70_90` | Holt | User-specified 70% and 90% intervals |
| `08_Holt_pseudo_OOS` | Holt | 4 pseudo-out-of-sample forecasts |

## Prerequisites and caveats

1. Install **MAR and Holt 2.0** in the same gretl environment as gretl4py.
   Their `Datavec.gdt` and `auspop.gdt` example datasets must be accessible
   using `--frompkg`. `MAR.zip` (not just a loose `.gfn`) may be required to
   obtain `Datavec.gdt`.
2. These fixtures have been checked against published signatures, **not
   executed** in gretl4py; some may fail for package data/dependency or
   version reasons. Inspect complete worker logs before changing the parser.
3. `MAR_IRFs` may render more than one chart or require a substantial number
   of iterations; let the runner count actual Matplotlib figures. Do not
   assume eight plots or increment audited totals before a full successful run.
4. Both packages own their plot creation; `PASS` proves execution and active
   Matplotlib figures, not pixel-perfect parity with gnuplot.

## Commands

From `gretl-gretl4py/tests/plots` after applying the initial patch:

```bash
python probe_package_batch.py --custom priority19 \
  --cases 01_MAR_LSE_GIRF_1_2 05_Holt_OOS_default \
  --approved --continue-on-error

python probe_package_batch.py --custom priority19 \
  --approved --continue-on-error
```

The latest subset run can overwrite `batch_results.csv` under
`package_plot_results/batch/custom/priority19/`; retain diagnostic logs.
