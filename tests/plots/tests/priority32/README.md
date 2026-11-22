# priority32 — final qualitative release gate for gptranslate 0.13

This batch tests four **distinct quality risks**, not four new grammar features.
It contains three previously untested packages and requires manual inspection
of all four rendered images. No local gretl/libgretl execution was available
while authoring these fixtures.

| Case | Package | Quality risk | Expected figures |
| --- | --- | --- | ---: |
| 01_Imputer_missing_backfill | Imputer | `NA` blocks, gaps versus continuous imputed data | 1 |
| 02_SETAR_threshold_LR | SETAR | LR sequence, critical line, dynamic `yrange` | 1 |
| 03_RollingStats_rolling_correlation | RollingStats | Correct placement of fixed-window statistics in calendar time | 1 |
| 04_RollingStats_expanding_AR1 | RollingStats | Correct placement of expanding-window AR estimates | 1 |

## Fidelity and dependencies

- Install complete `Imputer`, `SETAR`, and `RollingStats` packages through gretl,
  rather than copying their `.gfn` files. They are not installed by this patch.
- `Imputer` declares `extra`, `PanelTools`, and `StrucTiSM` as package
  dependencies. Install complete packages if the loader requires them. This
  case follows its original `bjg.gdt` missing-block example but calls public
  `impute(..., "backfill")`, avoiding a structural model fit.
- `SETAR` calls the public function and captures its **internally generated**
  confidence-profile plot. The original sample downloads `lynx.gdt` via HTTP;
  this case avoids the network and uses monthly changes in `bjg.gdt` instead.
  Bootstrap linearity testing is disabled: its 1000 repetitions are unrelated
  to the plot and increase variance and runtime.
- `RollingStats` derives its two cases from the package's official sample.
  The package exposes computation rather than a plotting API; both figures use
  the sample's native gretl `gnuplot` commands on real computed values.
- All cases intentionally retain `--output=display`. A command completing
  without a captured Matplotlib figure is **not** a PASS.

## Run four control cases

```bash
python probe_package_batch.py --custom priority32 \
    --approved --continue-on-error
```

## Manual image review

1. `Imputer`: the `gappy` line should disappear during 1954:3–1955:6,
   while `filled` remains continuous; dates must not shift.
2. `SETAR`: the dynamic y-range should not truncate the LR statistic or
   reference line; the original `--single-yaxis` semantics must be preserved.
3. Rolling correlation: the first plotted observation must correspond to a
   full 16-observation window, not the beginning of the `denmark` dataset.
4. Expanding AR(1): the first plotted estimate must align with the original
   sample window and later estimates must retain their correct dates.

These checks target visually silent errors. Do not change the native package
plotting code just to obtain a PASS. Classify failures as a package issue,
fixture issue, translation issue, or visual mismatch.
