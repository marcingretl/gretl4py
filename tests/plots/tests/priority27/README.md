# priority27 — thresh_infer and Threshold_Panel (experimental)

Eight reviewed plotting fixtures for two previously unaudited contributed
packages: `thresh_infer` v0.1 (2024-11-21; Hansen, 2000) and
`Threshold_Panel` v2.3 (2022-06-02; Hansen, 1999). They test **actual exported
package functions**, not invented stand-in plotting routines. The fixtures
have been inspected against the archived `.gfn` sources but have **not** been
executed with the user's gretl4py environment.

## Source provenance

- `audit/contributed/thresh_infer.zip`: `thresh_infer.gfn` sample script with
  `denmark.gdt`, `mg=diff(LRM)`, regressors `IBO IDE`, threshold `IDE`.
  The public `H_thresh_test()` (when `verbosity=1`) plots the sup-F profile;
  `H_thresh_estim()` (when `verbosity=1`) plots a threshold LR confidence
  interval. The package's second archived example uses an external HTTP data
  source and `SETAR`; these tests deliberately do **not** depend on either.
- `audit/contributed/Threshold_Panel.zip`: `Threshold_Panel.gfn` sample script
  and `ThresholdPanel_data_joe99.gdt`. The sample constructs lagged panel
  regressors and a threshold from `debta(-1)`, then estimates and plots
  `estimateTpm()` / `plotTpm()` results.

## Cases

| Case | Package | Intended graph |
| --- | --- | --- |
| `01_Hansen_test_homosked` | thresh_infer | sup-F threshold test, homoskedastic |
| `02_Hansen_test_heterosked` | thresh_infer | sup-F threshold test, heteroskedastic |
| `03_Hansen_estim_homosked` | thresh_infer | LR confidence profile, homoskedastic |
| `04_Hansen_estim_heterosked` | thresh_infer | LR confidence profile, heteroskedastic |
| `05_TP_single_time_effects` | Threshold_Panel | single-threshold LR with time effects |
| `06_TP_single_no_time_effects` | Threshold_Panel | single-threshold LR without time effects |
| `07_TP_single_alt_confidence` | Threshold_Panel | alternative confidence level and trimming |
| `08_TP_double_threshold` | Threshold_Panel | two estimated thresholds and their LR plots |

## Runtime and plotting details

- `Threshold_Panel` declares `needs-panel-data=true`; its `preload/` files
  initialize a balanced panel. The actual fixture reopens the bundled data,
  sets 15 periods and follows the original eight-unit sample restriction.
- Bootstrap repetitions are deliberately bounded: 49 for each `H_thresh_test`
  (instead of the default 1000), and 29 for each `Threshold_Panel` estimation
  (instead of the default 299). Threshold grid sizes of 80 replace the original
  full research replication. These choices test plotting, **not** precision
  of econometric confidence levels or published numerical replication.
- `H_thresh_test` and `H_thresh_estim` must retain `verbosity=1`: this is the
  switch that triggers the **package-native Gnuplot** plot. Unlike prior tests
  of package-estimated results, there is no additional artificial plot.
- `plotTpm` uses Gnuplot constructs such as `set for [i=1:5] linetype i
  dashtype i`, `set linetype`, `set key outside below`, and `set border ... ls`.
  These are worth checking in `gptranslate`; do not hide them or rewrite the
  package function to make a case pass.
- The package source contains `if exists(fname)` in `plotTpm()` even though
  the public argument is `path`. Do not silently patch upstream code. Run the
  test without supplying `path`, as in the package's own sample; if it fails,
  diagnose this separately from a translator limitation.
- No fixture requires `.bin` files, MPI, external HTTP or extra contributed
  dependencies. All cases use package data or standard gretl datasets.
- `PASS` is evidence of an active Matplotlib figure, **not** a pixel-by-pixel
  comparison with the original Gnuplot. Review the worker logs and PNGs before
  closing this priority group.

## Commands

From `gretl-gretl4py/tests/plots`:

```bash
python probe_package_batch.py --custom priority27 \
  --cases 01_Hansen_test_homosked 05_TP_single_time_effects \
  --approved --continue-on-error

python probe_package_batch.py --custom priority27 \
  --approved --continue-on-error
```

If the panel plot fails, inspect `logs/` first and check the original emitted
`gpttmp.plt` before altering the translator. In particular, a `FAIL` may arise
from a parser feature (including `set for ... linetype`) or package/runtime
code rather than the fixture.
