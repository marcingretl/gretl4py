# priority31 — novel gnuplot constructs for the gptranslate 0.13 release gate

This batch is deliberately **smaller** than priority29/30: six **new** graphics
paths from three previously untested contributed packages, selected for
syntax coverage rather than case counts. None of these fixtures has been
executed in gretl4py in the patch-authoring environment. A PASS means a figure
exists, not that its appearance is correct; review every PNG.

| Case | Package | Target syntax / behavior | Expected PNGs |
| --- | --- | --- | ---: |
| 01_BMA_inclusion_heatmap | BMA | `plot $map matrix with image`, binary palette, linked axes, secondary tics | 1 |
| 02_BMA_custom_histogram | BMA | `set style data histograms`, `clustered`, `using 2:xtic(1)` | 1 |
| 03_BMA_jointness_heatmap | BMA | diverging image palette, rotated labels, `set grid front` | 1 |
| 04_BMA_prediction_bars | BMA | `band-style=bars`, custom x-tics for out-of-sample prediction | 1 |
| 05_NIC_piecewise_gjr_garch | nic_gig | `f(x)` definitions, conditional `(x < 0) ? ... : ...`, `x**2`, `xzeroaxis` | 1 |
| 06_Wstats_weighted_boxes | wstats | `with boxes`, `set boxwidth ... relative`, translucent fill | 1 |

## Dependencies and package fidelity

- Install **complete** BMA, nic_gig and wstats packages via gretl, not loose
  `.gfn` copies; nic_gig additionally needs the complete `gig` package.
- BMA fixtures invoke the public `BMA()` and `BMA_Plot()` functions on observed
  `denmark` series. The batch-mode package writes timestamped `.plt` files, so
  a bounded search over the time interval occupied by `BMA_Plot()` locates
  and replays the *original* script. Each case renders a different package
  output. No plots or model results are fabricated.
- BMA `BMA_Plot()` uses a GUI-versus-batch branch, terminal directives and
  `catch plot`; the first execution may expose additional incompatibilities.
- nic_gig follows the official sample's two real GARCH model fits and calls
  the original `nic_plot_from_gig()`. It may require solver tuning and should
  **not** be replaced by a hand-authored gnuplot function plot.
- wstats follows the official sample's weighted-frequency plot block but
  constructs discrete groups and positive weights from real `denmark` data,
  avoiding reliance on other datasets. Its plotting block is native to the
  sample, not an exported package plotting API.
- BMA's custom inclusion prior (`theta`) must have regressor names, not
  just a sequence of three values; the histogram fixture assigns them.
- The BMA jointness variant may encounter undefined values for a small
  candidate model set; record numerical issues separately from translator
  errors. In particular, `NA` on the diagonal and equal palette stops must
  be handled by the translator rather than being replaced in the fixture.

## Control run (one BMA image, one function-only plot, one boxes plot)

```bash
python probe_package_batch.py --custom priority31 \
    --cases 01_BMA_inclusion_heatmap \
            05_NIC_piecewise_gjr_garch \
            06_Wstats_weighted_boxes \
    --approved --continue-on-error
```

## Full batch

```bash
python probe_package_batch.py --custom priority31 \
    --approved --continue-on-error
```

## Release gate

Classify each failure as: source package/dependency, hansl fixture, gptranslate
parse/IR/backend, or visual mismatch. Do not bypass unsupported constructs by
replacing actual package plot scripts. When 0.13 is frozen, preserve a regression
fixture for each newly supported syntax feature.
