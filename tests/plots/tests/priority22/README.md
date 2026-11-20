# priority22 — TenTS and basis_functions (experimental)

Eight reviewed plotting fixtures for two contributed packages, `TenTS`
and `basis_functions`. The scripts are derived from their **actual**
contributed archives and call their public functions: no mock plotting API.

## Provenance

- `audit/contributed/TenTS.zip`, archive dated 2026-08-30:
  `TenTS/examples/decomp.inp`, `TenTS/examples/trade_data.gdt`,
  `TenTS/TenTS.gfn` (its embedded `<sample-script>` and `tensor_plot()`).
- `audit/contributed/basis_functions.zip`, archive dated 2025-11-19:
  `basis_functions/basis_functions.gfn`, v1.1, embedded `<sample-script>`
  examples 1, 2, 3 and 5.

Do **not** extract or commit copies of `.gdt`, `.gfn` or `.pdf` files:
the test environment should find installed functions and sample datasets.
The TenTS original decompositions use the package's original sample without
removing existing comments; its two plots previously passed the broad audit.

## Cases

| Case | Package | Path exercised |
| --- | --- | --- |
| 01_TenTS_decomp_original | TenTS | Original `decomp.inp` with Tucker/CP/PCA; `gpbuild`, `gridplot`, `corr --plot` |
| 02_TenTS_tensor_plot_time | TenTS | Public `tensor_plot()`, no dimension labels, monthly country curves |
| 03_TenTS_tensor_plot_named | TenTS | Named dimensions; generated xtics, legend and `plot time_mat` |
| 04_TenTS_trade_grid | TenTS | Package-native `tensor_plot` four times inside `gpbuild`/`gridplot` |
| 05_basis_rcspline_logit | basis_functions | Embedded example 1: logit and `gnuplot` three series |
| 06_basis_rcspline_poisson | basis_functions | Embedded example 2: restricted Chebyshev splines and matrix plot |
| 07_basis_cspline_poisson | basis_functions | Embedded example 3: unrestricted cubic spline and matrix plot |
| 08_basis_fracpoly_logit | basis_functions | Embedded example 5: repeated fractional powers and logit plot |

## Prerequisites and limitations

- Both packages must be installed in the Python gretl runtime, including
  the `TenTS` example `trade_data.gdt`. Basic gretl datasets `mroz87.gdt`
  and `rac3d.gdt` must also be available.
- The fixtures have **not been executed** with the user's gretl4py build.
  They were checked against the archived source and package signatures.
- `TenTS` has a second official example using Tensor AutoRegression,
  but it does not produce any plots; it is not included here.
- A `PASS` confirms that a Matplotlib figure was generated. Compare the
  PNGs visually, especially case 03's labels and case 04's four panels.

## Commands

From `gretl-gretl4py/tests/plots`:

```bash
python probe_package_batch.py --custom priority22 \
  --cases 01_TenTS_decomp_original 05_basis_rcspline_logit \
  --approved --continue-on-error

python probe_package_batch.py --custom priority22 \
  --approved --continue-on-error
```

The runner's output directory is expected to be
`package_plot_results/batch/custom/priority22/`.
Do not change `gptranslate` unless the logs show a translator limitation.
