# priority14 — Clustering visualization and seasonal unit-root diagnostics

This maintained integration batch has **8 cases** across **2 packages**. The authoritative case identifiers and scripts are listed in `manifest.csv`.

**Themes:** Clustering; seasonality; unit roots.

| Case | Package | Plot / behavior exercised |
| --- | --- | --- |
| `01_kmeans_scatter` | `kmeans` | kmeans scatter — kmeans: separated synthetic clusters with reproducible observations. |
| `02_kmeans_scree` | `kmeans` | kmeans scree — kmeans: separated synthetic clusters with reproducible observations. |
| `03_kmeans_silhouette` | `kmeans` | kmeans silhouette — kmeans: separated synthetic clusters with reproducible observations. |
| `04_kmeans_silhouette_grid` | `kmeans` | kmeans silhouette grid — kmeans: separated synthetic clusters with reproducible observations. |
| `05_GHegy_fixed4_trend` | `GHegy` | GHegy fixed4 trend — GHegy: synthetic quarterly I(1) series, independent of external data. |
| `06_GHegy_bic8_dummies` | `GHegy` | GHegy bic8 dummies — GHegy: synthetic quarterly I(1) series, independent of external data. |
| `07_GHegy_fixed2_dummies` | `GHegy` | GHegy fixed2 dummies — GHegy: synthetic quarterly I(1) series, independent of external data. |
| `08_GHegy_bic6_trend` | `GHegy` | GHegy bic6 trend — GHegy: synthetic quarterly I(1) series, independent of external data. |

## Run this batch

From `tests/plots/` (with `gretl4py`, `gptranslate` and the complete required gretl packages installed):

```bash
python probe_package_batch.py --custom priority14 \
    --approved --continue-on-error
```

To run a single case without changing the stable fixture paths:

```bash
python probe_package_batch.py --custom priority14 \
    --cases 01_kmeans_scatter \
    --approved --continue-on-error
```

## Fixture conventions

- `manifest.csv` is authoritative; `samples/*.inp` contains the reviewed hansl input.
- `preload/`, if present, prepares package-specific data in an isolated process; `data/` contains intentionally bundled sample data.
- The batch runner executes each case in a separate Python process and writes logs/PNGs under `../../package_plot_results/batch/custom/priority14/`.
- `--approved` means third-party code is executed. Read scripts, package dependencies, network/file side effects before running.
- A PASS verifies a rendered figure, **not** its statistical correctness or visual equivalence to gnuplot.

See the [thematic index](../README.md) and the [plotting runner guide](../../README.md).
