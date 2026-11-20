# priority18 — package-output plotting (experimental)

Eight reviewed fixtures for two **new** packages, `StrucTiSM` and `FLS`.
These are deliberately marked **package-output plots** rather than
**package-native plot API** tests: `StrucTiSM` and `FLS` produce states or
coefficient matrices and the fixtures then use the native `gnuplot`,
`scatters`, `gpbuild` and `gridplot` commands. This tests integration of
package estimates with `libgretl` plotting and gptranslate, but **does not**
prove translation of a special package-emitted `.gp` script.

## Validated interfaces

- `STSM_setup(series y, bool epsilon, int trend, int slope, int seasonal, list X)`;
  `STSM_estimate(bundle *mod)`; `STSM_components(bundle mod, bool se)`;
  `LLT(series y, bool se, model *mod)`. These signatures and the airline
  example (`bjg.gdt`, `lg`, `lg_level`) are documented in *The StrucTiSM
  package for gretl*, v0.9.
- `FLS(series y, list X, bundle opts)` returns `coeff` as a `T x k` matrix,
  with option keys `mu`, `smooth`, `metric`, documented in *The FLS package*,
  v0.11. `mat2list(matrix, prefix)` comes from the gretl function reference.

Sources checked on 19 Nov 2026:
- https://gretl.sourceforge.net/current_fnfiles/unzipped/StrucTiSM.pdf
- https://gretl.sourceforge.net/current_fnfiles/unzipped/FLS.pdf
- https://gretl.sourceforge.net/gretl-help/funcref.html#mat2list

## Cases

| Case | Package | Goal |
| --- | --- | --- |
| `01_STSM_BSM_overlay` | `StrucTiSM` | BSM level vs observed series |
| `02_STSM_BSM_components` | `StrucTiSM` | BSM components as small multiple charts |
| `03_STSM_BSM_grid` | `StrucTiSM` | BSM three-panel gridplot from estimated states |
| `04_STSM_LLT_components` | `StrucTiSM` | LLT trend/slope component plots |
| `05_FLS_identity_coefficients` | `FLS` | Smoothed flexible LS coefficient paths, identity metric |
| `06_FLS_normalized_coefficients` | `FLS` | Smoothed flexible LS coefficient paths, normalized metric |
| `07_FLS_unsmoothed_coefficients` | `FLS` | Filtered flexible LS coefficient paths |
| `08_FLS_mu_comparison` | `FLS` | Two smoothing penalties compared in single figure |

## Assumptions and limitations

1. Both packages must already be installed for the Python gretl runtime.
   `bjg.gdt` is included in the usual gretl data package.
2. These fixtures were reviewed against documentation but **not executed**
   with the user's gretl4py build. In particular, check the actual installed
   FLS version and `mat2list` dimensions if any worker fails.
3. A `PASS` means a Matplotlib figure exists, **not** a visual comparison
   with native gnuplot. Do not change `gptranslate` until worker logs establish
   a genuine parser or backend limitation.

## Commands

From `gretl-gretl4py/tests/plots`:

```bash
python probe_package_batch.py --custom priority18 \
  --cases 01_STSM_BSM_overlay 05_FLS_identity_coefficients \
  --approved --continue-on-error

python probe_package_batch.py --custom priority18 \
  --approved --continue-on-error
```

The runner's default output directory is
`package_plot_results/batch/custom/priority18/`.
Focused reruns may overwrite the previous `batch_results.csv`.
