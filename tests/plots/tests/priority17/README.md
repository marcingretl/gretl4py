# priority17 — contributed-package plotting probes (initial, unrun)

Eight independently executed, reviewed graph-producing fixtures for **gig** and
**CUB**, which have not yet been covered by the maintained priority5–16 plot
suites. The verified baseline is **104/104 PASS, 123 figures, 46 packages**.
These eight cases are exploratory and have **not** been executed in the
maintainer's environment. Do not count them as PASS until confirmed.

| Case | Package | Plot path |
| --- | --- | --- |
| `01_gig_garch_volatility` | `gig` | Standard GARCH residuals and conditional volatility |
| `02_gig_garch_density` | `gig` | Standard GARCH fitted vs empirical innovation density |
| `03_gig_gjr_volatility` | `gig` | GJR residuals and conditional volatility |
| `04_gig_gjr_density` | `gig` | GJR fitted vs empirical innovation density |
| `05_CUB_officeho_distribution` | `CUB` | CUB(0,0) fitted rating probabilities vs observed frequencies |
| `06_CUB_informat_distribution` | `CUB` | Another CUB(0,0) ordinal response |
| `07_CUB_freqserv_conditional` | `CUB` | CUB(1,1) predicted frequencies by binary group |
| `08_CUB_freqserv_average` | `CUB` | CUB(1,1) unconditional marginal probabilities |

## Public API grounding and assumptions

The in-tree `gig` addon exposes `gig_setup`, `gig_estimate`, `gig_plot` and
`gig_dplot`. Its documentation demonstrates `gig_setup(Y, 1, const)` for a
standard GARCH model and `gig_setup(Y, 3, const)` for GJR. The plotting
functions take a **bundle pointer** and return `void`; the optional bundle
supports `title` on `gig_plot` and `fontsize` on either graph path. The
fixture opens the distributed `b-g.gdt` data file and uses the series `Y`.

The installed contributed package `CUB` **2.0** (2026-05-19) defines
`CUB_setup(series y, bundle opts[null])`; older documentation may show the
former multi-argument signature `CUB_setup(y, Y, W, m)`. The current API
passes the uncertainty and feeling covariate lists as `opts.Y` and `opts.W`:

```hansl
list Y = freqserv
list W = freqserv
bundle opts = _(Y=Y, W=W)
bundle mod = CUB_setup(officeho, opts)
CUB_estimate(&mod)
```

The function automatically prepends the constant to both lists, giving a
CUB(1,1) specification. The installed package's bundled sample script uses
the same options-bundle API. `CUB_dplot(mod)` draws fitted/observed ordinal
probabilities for a CUB(0,0), or average fitted probabilities for CUB(1,1);
`CUB_dplot(mod, freqserv)` draws the fitted distributions for the two levels
of the binary `freqserv` indicator. `CUB_dplot` takes the bundle **by value**,
unlike the estimation function's bundle pointer. All examples use the bundled
`univer.gdt` and variables `officeho`, `informat`, `freqserv`.

The initial `07` and `08` fixtures used the old API and failed before
plotting; this correction needs an isolated rerun before claiming PASS.

Upstream references:
- https://fossies.org/linux/gretl/addons/gig/doc/gig.tex
- https://fossies.org/linux/gretl/addons/gig/gig.spec
- https://gretl.sourceforge.net/current_fnfiles/unzipped/CUB.pdf

## Installation and execution

- Make sure `gig` (standard gretl addon) and the contributed `CUB` function
  package are installed and includeable by gretl4py. CUB's auxiliary sample
  `univer.gdt` must be installed with the package for `--frompkg=CUB` to work.
- Each case has a `preload/CASE.inp` file for the audit runner's existing
  dataset-before-include hook. No changes to the runner or libgretl are needed.
- Existing global numerical settings and dependencies may affect estimation;
  a failure must first be diagnosed via the worker log, **not** treated as a
  confirmed translator defect.
- `PASS` establishes successful script execution and creation of a Matplotlib
  figure. It does not guarantee visual equivalence to gnuplot or econometric
  validity of estimates.

Start with a basic GARCH plot and a simple ordinal distribution:

```bash
python probe_package_batch.py --custom priority17 \
  --cases 01_gig_garch_volatility 05_CUB_officeho_distribution \
  --approved --continue-on-error
```

Then the other six:

```bash
python probe_package_batch.py --custom priority17 \
  --cases 02_gig_garch_density 03_gig_gjr_volatility \
          04_gig_gjr_density 06_CUB_informat_distribution \
          07_CUB_freqserv_conditional 08_CUB_freqserv_average \
  --approved --continue-on-error
```

Finally, once any failures have been classified or corrected:

```bash
python probe_package_batch.py --custom priority17 \
  --approved --continue-on-error
```

Reports: `package_plot_results/batch/custom/priority17/`.
Focused runs can overwrite the latest `batch_results.csv`.
