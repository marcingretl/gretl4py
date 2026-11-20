# priority25 — GFE and dcm (experimental)

Eight source-reviewed plotting fixtures for the contributed **GFE** (grouped
fixed effects) and **dcm** (discrete choice models) packages. The tests use
functions and data in the archived package sources rather than constructing
mock plots. They have **not** yet been executed with the user's gretl4py
runtime; run the two control cases before the complete batch.

## Source provenance

- `audit/contributed/GFE.zip`: `GFE/GFE.gfn` v0.1 (2026-07-28),
  `GFE/examples/BonMan_replication.inp`, and the package's
  `Acemoglu_etal.gdt` (90 units × 7 periods, 630 complete observations).
- `audit/contributed/dcm.zip`: `dcm/dcm.gfn` v0.9,
  `dcm/examples/{meff_sample,fishing,sample_wide}.inp`, and the
  `tunafish.gdt`, `fishing.gdt`, `tunafish_small_wide.gdt` data files.
- Package functions and bundled data are not modified by these fixtures.

## Cases

| Case | Package | Chart and source path |
| --- | --- | --- |
| `01_GFE_timevarying` | GFE | `group_plot(mod)` time-varying group paths |
| `02_GFE_timeinvariant` | GFE | `group_plot(mod)` time-invariant group boxes |
| `03_GFE_effect_distribution` | GFE | `group_fx(mod)` plus native distribution boxplot |
| `04_GFE_BIC_groups` | GFE | BIC selection (bounded) and package group paths |
| `05_dcm_meff_sample` | dcm | Original factorized income marginal-effect boxplot |
| `06_dcm_fishing_probabilities` | dcm | Fishing conditional-logit probabilities from `bun.phat` |
| `07_dcm_wide_probabilities` | dcm | Wide-format mixed logit alternative probabilities |
| `08_dcm_wide_income_effects` | dcm | Wide-format `dcm_meff("income", bun)` effect paths |

## Audit adaptations and limits

- `GFE` samples reduce group count from 15 in the original Bonhomme–Manresa
  replication to 3, and use `randstarts=2` or `4`, `parallel=0` to avoid MPI
  and costly random-start searches. Case 04 retains the documented BIC
  selection path with a maximum of 3 groups. These cases **do not replicate**
  the published econometric results. The original panel data remain unchanged.
- `GFE` has `needs-panel-data="true"`: the preload scripts initialize a small
  balanced panel before the runner loads the package. The actual sample
  dataset is then read with `--frompkg=GFE`.
- Case 05 retains the original `meff_sample.inp` script, including its
  income-effect extraction and factorized boxplot. Cases 06–08 build new
  figures from *actual* package results (`bun.phat`, `dcm_meff()`).
  Where an estimate is naturally a matrix, the last plotting column is an
  explicit observation index, following gretl's `gnuplot --matrix` rule:
  earlier columns are y variables and the final column is x. This avoids
  accidentally using a probability or marginal effect as the x variable.
- No examples access external HTTP endpoints, `.bin` databases, or call MPI.
  The runner loads packages from its existing local cache. The experiments
  may produce different estimated coefficients across seeds/solver versions.
- `PASS` means at least one Matplotlib figure was produced with no reported
  errors, not pixel-exact correspondence with gnuplot. Visual inspection is
  required before closing the priority group. Failures must be diagnosed
  from worker logs, not automatically attributed to gptranslate.

## Run

From `gretl-gretl4py/tests/plots`:

```bash
python probe_package_batch.py --custom priority25 \
  --cases 01_GFE_timevarying 05_dcm_meff_sample \
  --approved --continue-on-error

python probe_package_batch.py --custom priority25 \
  --approved --continue-on-error
```

Output directory: `package_plot_results/batch/custom/priority25/`.
