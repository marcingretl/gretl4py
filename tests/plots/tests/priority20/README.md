# priority20 — package-native plot syntax and posterior histograms (experimental)

Eight reviewed **but not yet executed** integration tests. This batch uses two
already documented native plotting interfaces: `ADMBP` (new to the maintained
suite) and `BVAR` (already covered in `priority16`; this suite adds the distinct
`BVAR_post_plot()` coefficient-histogram path). Only **one** previously untested
package is introduced; if the full suite passes, the distinct-package total
will be 52, not 53.

## Coverage

| Case | Package | What the package draws |
| --- | --- | --- |
| `01_ADMBP_irf_point_only` | ADMBP | Single-column dynamic multiplier path |
| `02_ADMBP_irf_with_bounds` | ADMBP | Three-column central path and bounds |
| `03_ADMBP_irf_negative_bounds` | ADMBP | Negative path and zero-crossing band |
| `04_ADMBP_irfadv_custom` | ADMBP | `irf_plotadv()`, fontsize/linewidth and `display` target |
| `05_BVAR_coeff_a_grid` | BVAR | Grid of first equation's coefficient histograms |
| `06_BVAR_coeff_b_grid` | BVAR | Grid of second equation's coefficient histograms |
| `07_BVAR_coeff_a_separate` | BVAR | Individual histograms without grid |
| `08_BVAR_coeff_all_grid` | BVAR | Full two-equation coefficient histogram grid |

## Public APIs and inputs

ADMBP (package catalog version 1.0): the published PDF documents
`irf_plot(matrix m, string title, string xlab, string ylab)` with either one
column or three columns (central response and confidence limits), and
`irf_plotadv(matrix m, string fname, string title, string xlab, string ylab,
int fontsize, scalar lw)`. In the current public PDF the heading for the
latter function contains a probable *copy/paste typo* saying `irf_plot`;
the example and contents use the correct `irf_plotadv` name. The three-column
matrices here are **synthetic, hand-constructed plot inputs**, not confidence
intervals estimated by bootstrap. The `display` target in the advanced path
must be validated at runtime.

BVAR 0.4: `BVAR_setup(Y,1,"diff")` followed by
`BVAR_posterior(mod,"coeff",_(iter=399,summ_only=0,mpi_np=1))` and
`BVAR_post_plot(post,"coeff",selection,_(grid=...,gridfontsize=...))`.
Unlike `BVAR_fcast_plot` and `BVAR_irf_plot` already tested in `priority16`,
`BVAR_post_plot` generates coefficient histograms and **requires
`summ_only=0`** to retain the posterior draws. We deliberately use 399
iterations for a fast, deterministic *smoke test*, not statistical inference.

Public documentation (accessed 20 November 2026):
- https://gretl.sourceforge.net/current_fnfiles/unzipped/ADMBP.pdf
- https://gretl.sourceforge.net/current_fnfiles/unzipped/BVAR.pdf
- https://gretl.sourceforge.net/cgi-bin/gretldata.cgi

## Assumptions and caveats

1. Install the `ADMBP` package and ensure `BVAR` version 0.4 is available
   to the same gretl installation used by gretl4py. `ADMBP.zip` may be
   needed if the bare GFN is not available separately.
2. These scripts were checked against published documentation, but were
   **not executed** in gretl4py. Inspect the actual worker logs before
   attributing a failure to `gptranslate`.
3. The `ADMBP` advanced plot might treat `display` differently from the
   simple plot. If no Matplotlib figure is produced, check its real output
   specification rather than modifying the translator prematurely.
4. `PASS` means an active Matplotlib figure was produced. It does not prove
   pixel-identical rendering compared with gnuplot.
5. Figure counts can exceed one per case, notably for `grid=0` in the BVAR
   histogram path. Use counts from the actual runner.

## Commands

Run from `gretl-gretl4py/tests/plots` after applying the patch:

```bash
python probe_package_batch.py --custom priority20 \
  --cases 01_ADMBP_irf_point_only 05_BVAR_coeff_a_grid \
  --approved --continue-on-error

python probe_package_batch.py --custom priority20 \
  --approved --continue-on-error
```

Reports go under `package_plot_results/batch/custom/priority20/`. A
subset rerun can overwrite `batch_results.csv` for this directory.
