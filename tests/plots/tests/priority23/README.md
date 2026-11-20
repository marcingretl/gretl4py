# priority23 — ETS and BACE (experimental)

Eight reviewed plotting fixtures for two **previously untested in custom
priority manifests** contributed packages, `ETS` and `BACE`. Each fixture is
based on an example in the archived contributed package; package functions
rather than simulated plotting stubs generate the charts.

## Source provenance

- `audit/contributed/ETS.zip`: `ETS/ETS.gfn` (v1.0, 2026-11-07),
  `ETS/examples/{confidence,ema,Holt,Winters}.inp`.
- `audit/contributed/BACE.zip`: `BACE/BACE.gfn` (v3.2, 2026-11-11),
  `BACE/examples/{FLS_41t72,interval,logit,tobit}.inp`.
- Original example comments and computations are retained; the BACE
  examples have explicit, documented plotting-audit adaptations.

## Cases

| Case | Package | What it checks |
| --- | --- | --- |
| 01_ETS_confidence | ETS | Alternative 70/90% confidence intervals via `ETS_plot()` |
| 02_ETS_ema | ETS | Simple exponential smoothing and plot |
| 03_ETS_Holt | ETS | Estimated versus supplied Holt parameters and forecast plots |
| 04_ETS_Winters | ETS | Additive and multiplicative seasonal ETS plots |
| 05_BACE_FLS_growth_subset | BACE | Growth BACE posterior diagnostics with variable inclusion / jointness |
| 06_BACE_interval | BACE | Interval-regression model averaging and posterior plot family |
| 07_BACE_logit_jointness | BACE | Logistic BACE, MYQ jointness and custom prior inclusion probabilities |
| 08_BACE_tobit | BACE | Tobit model averaging and posterior graphics |

## Audit-specific adaptations

The BACE original `FLS_41t72.inp` runs **3 million MC3 draws** with 41
regressors. Case 05 instead uses variables `2..14` and `Nrep=5000`; this is
**not** a numerical replication of the published BACE example. The other
BACE cases also use `Nrep=5000` instead of the package defaults, and all
explicitly set `do_plot=1, show_progress=0`. These changes keep the same
public plotting code paths but make the integration probe bounded. Jointness
and posterior estimates depend on the reduced MC3 sample and should not be
interpreted scientifically. Fixed random seeds from the original examples
are preserved.

## Prerequisites

- Both packages installed in the gretl runtime; the runner uses its standard
  custom manifest/preload/sample mechanism.
- Stock gretl data files `australia.gdt`, `data3-6.gdt`, `data9-3.gdt`,
  `wtp.gdt`, and `ETS`/`BACE` example data provided by their packages.
- These fixtures are **source-reviewed only**. They have not been executed
  in the user's gretl4py environment. Inspect logs on failure before editing
  the translator or any package code.
- PASS signals one or more active Matplotlib figures, not pixel-identical
  equivalence with gnuplot. Inspect complex BACE figures visually after PASS.

## Run

From `gretl-gretl4py/tests/plots`:

```bash
python probe_package_batch.py --custom priority23 \
  --cases 01_ETS_confidence 08_BACE_tobit \
  --approved --continue-on-error

python probe_package_batch.py --custom priority23 \
  --approved --continue-on-error
```

Report directory: `package_plot_results/batch/custom/priority23/`.
