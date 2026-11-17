# priority6 — proposed contributed-package plot probes

This suite uses the existing `probe_package_batch.py --custom` runner. The
fixtures use deterministic synthetic data and only local installed gretl
function packages; no data is downloaded or modified. They were reviewed
against the public function signatures and plotting paths in the archived
contributed `.gfn`/`.zip` packages. They have **not yet been executed** in a
real gretl4py environment; a first run may identify test-input errors or new
`gptranslate` compatibility gaps. Neither outcome should be silently hidden.

| Case | Package | Graph exercised |
| --- | --- | --- |
| `01_Brown_linear` | Brown | Linear smoothing time-series plot |
| `02_Brown_quadratic` | Brown | Quadratic smoothing time-series plot |
| `03_Holt_forecast` | Holt | Forecast/fitted path |
| `04_Holt_components` | Holt | `tsplots` components |
| `05_BreitungCandelonTest` | BreitungCandelonTest | Matrix-based frequency-domain test curve |
| `06_naiveFC` | naiveFC | Random-walk forecast plot |
| `07_random_forest` | random_forest | R-backed variable-importance plot, `with boxxy` |

Run one case at a time initially so failures can be diagnosed without a long
batch. From `tests/plots`:

```sh
python probe_package_batch.py --custom priority6 --cases 01_Brown_linear --approved
```

After individual tests are reviewed, run:

```sh
python probe_package_batch.py --custom priority6 --approved --continue-on-error
```

Outputs are written under
`package_plot_results/batch/custom/priority6/` and do not overwrite the
baseline 95-package audit or `priority5` results. As in all custom suites,
`PASS` indicates successful figure creation, **not** exact visual equivalence.

`session.inp` is a session artifact and is always excluded from the installed
example discovery flow; this custom suite does not reference it.
