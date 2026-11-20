# priority24 — DFM and DCC package plots (experimental)

Eight source-reviewed integration fixtures for the contributed `DFM` (dynamic
factor models) and `DCC` (dynamic conditional correlations) packages. These
scripts exercise actual functions of the archived packages, plus native
`gnuplot`, `scatters`, `gpbuild` and `gridplot` output where appropriate.

## Source provenance

- `audit/contributed/DFM.zip`: `DFM/DFM.gfn` v0.5 (2023-07-09) and
  `DFM/examples/Labor.inp` + `DFM/examples/Labor.gdt`. Test 01 retains the
  original Labor plot workflow; tests 02–04 call `DFM_Plot(&Mod, ptype)`.
- `audit/contributed/DCC.zip`: `DCC/DCC.gfn` v0.1 (2023-03-10), its embedded
  sample script, `DCC/examples/{artif,Indices}.inp`, and
  `DCC/examples/{IT_companies,MktIndices}.gdt`. Tests 05–08 derive from these
  and plot the `DCC_GetCorrel()` series.
- Package implementations and all original source files are unchanged.

## Cases

| Case | Package | Plotting path |
| --- | --- | --- |
| `01_DFM_Labor_smoothed` | DFM | Original QML/EM labor common-component overlay |
| `02_DFM_PCA_factors` | DFM | `DFM_Plot(&Mod, 0)` factor small multiples |
| `03_DFM_PCA_scree` | DFM | `DFM_Plot(&Mod, 1)` scree with package styles and objects |
| `04_DFM_PCA_heatmap` | DFM | `DFM_Plot(&Mod, 2)` matrix/image heatmap |
| `05_DCC_IT_sample` | DCC | Embedded sample: GARCH/DCC then `scatters` correlations |
| `06_DCC_artificial_line` | DCC | Simulated innovations, DCC and combined line plot |
| `07_DCC_indices_correlations` | DCC | Three market indices, targeted DCC, line plot |
| `08_DCC_IT_grid` | DCC | Three pairwise conditional correlations via `gpbuild`/`gridplot` |

## Audit adaptations and scope

- `DFM` cases 02–04 use PCA-only estimation (`method=1`) on the stock Labor
  package dataset, avoiding the external `staticfactor` dependency and
  `fedstl.bin` required by another official sample. Case 01 preserves the
  original Labor example's iterative method and its sample comments.
- `DCC` case 05 follows the package's embedded IT-companies sample. Case 06
  shortens `artif.inp` from 1024 to 600 observations and uses 3 instead of 5
  innovations. Case 07 uses 3 of 4 `Indices.inp` market indices, starts in
  2014 and adds plotting. Case 08 plots the three IT-companies correlations
  in a grid. These are **plotting tests**, not econometric replications.
- The runner must have these contributed packages installed (or access to
  the local archive cache under `audit/contributed`). It loads the named
  package before invoking each fixture. Each fixture also uses the exact
  original `include <package>.gfn` syntax as in the archive examples.
- No external networking, executable subprocesses, or `.bin` database access
  is required by these scripts. `DCC_Step1()` fits univariate GARCH models and
  `DCC_Estimate()` optimizes a multivariate likelihood, so this group may take
  more computation than earlier plotting batches.
- The fixtures have been inspected structurally but **not executed** with the
  user's gretl4py environment. A `PASS` proves figures were returned, not
  numerical or pixel-identical agreement with gnuplot. Review worker logs
  before attributing a failure to the translator.

## Commands

From `gretl-gretl4py/tests/plots`:

```bash
python probe_package_batch.py --custom priority24 \
  --cases 01_DFM_Labor_smoothed 05_DCC_IT_sample \
  --approved --continue-on-error

python probe_package_batch.py --custom priority24 \
  --approved --continue-on-error
```

Report directory: `package_plot_results/batch/custom/priority24/`.
