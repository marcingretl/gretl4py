# priority30 — five contributed package audits (source-reviewed)

Twenty cases, four per package. These scripts were prepared by inspecting the
real `.gfn` source and original `<sample-script>` from
`priority30_sources.tar.gz`. They have **not** been executed in gretl4py here;
`git apply --check` and static review are not evidence of runtime success.

## Packages and expected graphics

| Package | Cases | Basis | Expected PNGs |
| --- | --- | --- | ---: |
| `delayspectral` 1.0 | 01–04 | `phasedelay()` and built-in `hall` observations; two package plots per call | 8 |
| `VSG_test` 1.3 | 05–08 | `VSG_test()` with built-in `np.gdt`; original fitted deterministic-trend plot | 4 |
| `uniFCextensions` 1.2 | 09–12 | `GUI_uniFCboot()` using active `$model`; native `plot_multibands()` | 4 |
| `carr` 0.81 | 13–16 | `carr()` + `carr_plot_R()` or `carr_plot_density()` | 4 |
| `TsCrossValidation` 0.3 | 17–20 | `crossvalid()` + original sample's matrix plot construction | 4 |

**Expected total:** 24 figures if all package functions complete successfully.

### Requirements and fidelity

- Install **complete contributed packages** through Gretl/SourceForge. Never
  copy bare `.gfn` files into the installed package directory.
- These five packages were selected after checking prior manifests, which
  exclude them from `priority5`–`priority29` in the provided archive.
- `delayspectral` uses the packaged sample's `hall`, `consrat`, `ewr`, and
  optional `vwr` conditioning variables. Its `phasedelay()` intentionally
  produces two graphs, including numerical frequency-domain checks.
- `VSG_test` uses `np.gdt` and `rgnp`, as in the package example; the four
  cases vary significance, trimming bound, and drift-only vs drift-break.
- `uniFCextensions` uses the **public GUI wrapper** `GUI_uniFCboot()` immediately
  after OLS to access `$model`. All runs use the package's own bootstrap and
  graphical forecast-band implementation, with 299 replications; review
  interval semantics carefully. The final case exponentiates `LRY`, which
  is an observed logarithmic series. In some execution contexts the wrapper
  may not have access to `$model`; if so inspect the diagnostic rather than
  replacing the graph with synthetic Python or gnuplot output.
- `carr`'s original sample opens `2005-CARR.gdt` over HTTP. To keep these
  fixtures offline and reproducible, we use built-in `nysewk.gdt` close
  prices and form a positive scaled absolute log-return proxy. This is **not** a
  real high/low trading range; treat it only as an execution and graphics
  stress test, not an econometric validation of CARR. The two distributions
  may exhibit numerical convergence issues.
- `TsCrossValidation` has no plotting function. The four cases genuinely
  compute `crossvalid()` output for OLS/Probit and plot it using the
  **same command structure as its official sample**. No fabricated CV
  values or stand-in charts are used.
- A `PASS` means a Matplotlib figure was produced, **not** that semantics
  and visual appearance have been verified. Review every generated PNG.

## Five control cases

```bash
python probe_package_batch.py --custom priority30 \
    --cases 01_Delay_default_AR34 05_VSG_sample_break_5pct \
            09_UniFC_AR1_two_bands 13_CARR_exponential_path \
            17_TSCV_OLS_MAE \
    --approved --continue-on-error
```

## Full batch

```bash
python probe_package_batch.py --custom priority30 \
    --approved --continue-on-error
```

If a package fails, inspect the corresponding full diagnostic log first and
classify the failure (package/data, hansl, gptranslate parser, figure quality).
Do not modify upstream graphics into stand-in plots to force a PASS.

## Compatibility notes for batch execution

- `VSG_test` produces an authentic `gpttmp01.plt` file when run in batch mode.
  The sample replays that file with `gnuplot --input=... --output=display`;
  no substitute plot or rewritten package formula is used.
- `uniFCextensions` exposes plotting only through the GUI wrapper, which cannot
  access `$model` in a script. The samples instead call the documented
  `uniFCboot()` API, then invoke the unchanged package `plot_multibands()`
  helper. For this test, the helper must be exported (see companion one-line
  patch to `uniFCextensions.gfn`).
- `carr` must be version 0.81, not the older 0.6 whose `if ARCH=0` fails
  during parsing. Install the complete updated package before rerunning 13-16.

- `uniFCextensions` uses a native `plot ... end plot` block without
  `--output=display`. In batch mode it saves `gpttmp01.plt` instead of
  producing an active Matplotlib figure. Tests 09–12 replay the original
  script with `gnuplot --input="gpttmp01.plt" --output=display`, as in
  the `VSG_test` fixtures; forecast data and styles are unchanged.
