# priority26 — cffilt and Canova_Hansen (experimental)

Eight source-reviewed plotting fixtures for the contributed `cffilt` (v2.2,
2024-03-19) and `Canova_Hansen` (v1.3, 2023-12-12) packages. All tests are
based on real exported package functions. They have **not** yet been executed
in the user's gretl4py environment.

## Source provenance

- `audit/contributed/cffilt.zip`: `cffilt.gfn`, original
  `examples/artificial.inp`, `examples/frbatlanta.inp`,
  `examples/replication.inp`, and annual `examples/gdp.gdt` (1959–1999).
- `audit/contributed/Canova_Hansen.zip`: `Canova_Hansen.gfn`, its embedded
  `data9-9.gdt` example, `CH_test_bundle_plot()` API, and 16 compressed
  coefficient matrices in `coeffs/` used to compute p-values.
- `bjg.gdt` and `data9-9.gdt` are normal gretl example datasets. Package
  archives must be installed by the probe's existing local cache mechanism.

## Cases

| Case | Package | Purpose |
| --- | --- | --- |
| `01_cffilt_artificial_spectra` | cffilt | Original synthetic spectrum: `pergm`, `gpbuild`, `gridplot` |
| `02_cffilt_annual_GDP` | cffilt | Annual GDP CF cycle/trend and two line series |
| `03_cffilt_monthly_list` | cffilt | Monthly `cffilt_list()` on two series |
| `04_cffilt_bands_grid` | cffilt | Four panels with three CF frequency bands |
| `05_CH_dummies_residuals` | Canova_Hansen | Seasonal dummies, levels, auxiliary residuals |
| `06_CH_trigonometric_fitted` | Canova_Hansen | Trigonometric terms, levels, fitted vs actual |
| `07_CH_diff_dummies_residuals` | Canova_Hansen | Seasonal dummies, first differences, trend |
| `08_CH_diff_trig_fitted` | Canova_Hansen | Trigonometric terms, first differences, lagged dependent series |

## Intent and limits

- The `cffilt` spectral example keeps the actual algorithm and `gpbuild`
  layout from `artificial.inp`; it omits `include cffilt.gfn` because the probe
  loads the package first. Case 02 uses the included annual GDP dataset,
  replacing a text-only example with a plot of the actual returned cycle.
  Cases 03–04 use standard `bjg.gdt` in place of `fedstl.bin`, and call the
  actual public `cffilt_list()` and `cffilt()` interfaces.
- `Canova_Hansen` tests vary the documented `diffy`, `sdum` and `trendroot`
  parameters. Every case calls the package's `CH_test_bundle_plot()` in one
  of its two modes, then explicitly draws the same returned series using
  `gnuplot --output=display`. **Important:** the package's own bundle-plot
  function omits `--output=display`. In a batch session its plot may only be
  written to a `.plt` file rather than creating an active Matplotlib figure;
  the explicit plot guarantees a visible figure. Consequently a `PASS` in
  cases 05–08 establishes plotting of *package-estimated results*, not
  necessarily that the native bundle-plot function displayed the figure.
- The 16 matrix files are an essential dependency of `Canova_Hansen`, which
  computes seasonal-test p-values via them. The probe should load the package
  including its data subdirectory. Do not extract or modify them separately.
- No cases require remote network access, MPI or `.bin` databases. Cases
  02–04 and 05–08 are adapted tests, not literal replications of published
  examples. `PASS` means a Matplotlib figure was created without runtime
  errors; it does not imply reference-pixel identity with gnuplot.

## Execution

From `gretl-gretl4py/tests/plots`:

```bash
python probe_package_batch.py --custom priority26 \
  --cases 01_cffilt_artificial_spectra 05_CH_dummies_residuals \
  --approved --continue-on-error

python probe_package_batch.py --custom priority26 \
  --approved --continue-on-error
```

Output directory: `package_plot_results/batch/custom/priority26/`.
Review worker logs and visual results before a final commit. If a test fails,
first determine whether the failure is in the fixture, source-package runtime
or gptranslate; do not change the translator based on a `FAIL` alone.
