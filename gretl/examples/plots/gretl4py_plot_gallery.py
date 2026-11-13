#!/usr/bin/env python3
"""
gretl4py plotting gallery and integration test.

This script exercises plotting paths through gretl4py -> libgretl ->
gptranslate -> Matplotlib. Its two modes use the SAME plot definitions:

  demo  - show one figure at a time; close it before continuing
  test  - use the non-interactive Agg backend, validate and close figures

The examples intentionally combine real data (denmark.gdt) with a few
reproducible synthetic series so that colors, bands, boxes and stepped
plots are easier to see. All comments are written in English.

Examples:
  python gretl4py_plot_gallery.py --list
  python gretl4py_plot_gallery.py --mode demo --limit 10
  python gretl4py_plot_gallery.py --mode demo --group matrix
  python gretl4py_plot_gallery.py --mode test
  python gretl4py_plot_gallery.py --mode test --save-dir plot-gallery
  python gretl4py_plot_gallery.py --mode test --filter band --verbose

Requirements: gretl4py with gptranslate support, Matplotlib and NumPy.
The installed gretl4py must include the current model.plot and Matrix plotting APIs.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys
import traceback
from typing import Any, Callable

import numpy as np


@dataclass(frozen=True)
class PlotCase:
    """One independently reportable integration-test case."""

    id: str
    group: str
    title: str
    make: Callable[["Context"], Any]


class Context:
    """Lazily create shared real data, models and matrix fixtures."""

    def __init__(self, gretl: Any):
        self.gretl = gretl
        self._data = None
        self._ols = None
        self._var = None
        self._matrix = None
        self._matrix_band = None
        self.series: dict[str, np.ndarray] = {}

    @property
    def data(self):
        if self._data is None:
            # Load the actual Gretl dataset used in the package examples.
            d = self.gretl.get_data("denmark")
            y = np.asarray(d.get_series("LRY").numpy_view, dtype=float).reshape(-1)
            n = y.size
            if n < 24:
                raise RuntimeError("denmark dataset is unexpectedly short")

            # Generate deterministic secondary series for visual variety.
            t = np.linspace(0.0, 6.0 * np.pi, n)
            rng = np.random.default_rng(20261113)
            noise = rng.normal(0.0, 0.18, n)
            wave = np.sin(t) + 0.25 * np.cos(2.3 * t) + noise
            signal = 0.55 * wave + 0.35 * np.cos(0.45 * t) + noise * 0.3
            positive = np.exp(0.42 * wave + 0.15 * rng.standard_normal(n))
            steps = np.repeat(np.arange((n + 6) // 7), 7)[:n].astype(float)
            pulses = np.maximum(0.0, np.sin(t) + 0.15)

            self.series = {
                "demo_wave": wave,
                "demo_signal": signal,
                "demo_positive": positive,
                "demo_steps": steps,
                "demo_pulses": pulses,
            }
            for name, values in self.series.items():
                d.add_series(np.asarray(values, dtype=np.float64), name)
            self._data = d
        return self._data

    @property
    def ols(self):
        if self._ols is None:
            # Trigger dataset creation before fitting; estimators use the
            # current libgretl dataset associated with the Python session.
            _ = self.data
            self._ols = self.gretl.ols("LRM 0 LRY IBO IDE").fit()
        return self._ols

    @property
    def var(self):
        if self._var is None:
            _ = self.data
            # This is the established 4-variable Denmark VAR example.
            self._var = self.gretl.var(4, "1 2 3 4").fit()
        return self._var

    @property
    def matrix(self):
        if self._matrix is None:
            _ = self.data
            columns = [self.series[k] for k in
                       ("demo_wave", "demo_signal", "demo_positive", "demo_steps")]
            self._matrix = self.gretl.Matrix(np.column_stack(columns))
        return self._matrix

    @property
    def matrix_band(self):
        if self._matrix_band is None:
            _ = self.matrix
            # libgretl bandmat contains [band center, half-width].
            bandmat = self.gretl.Matrix(np.column_stack((
                self.series["demo_wave"],
                np.full(self.series["demo_wave"].size, 0.35),
            )))
            band = self.gretl.Bundle()
            band["bandmat"] = bandmat
            band["factor"] = 1.0
            band["style"] = "fill"
            self._matrix_band = band
        return self._matrix_band


def band(gretl: Any, center: str, width: float, style: str, title: str):
    """Construct a libgretl graphical band specification."""
    b = gretl.Bundle()
    b["center"] = center
    b["width"] = width
    b["factor"] = 1.0
    b["style"] = style
    b["title"] = title
    return b


def single_band(c: Context):
    return band(c.gretl, "demo_wave", 0.35, "fill", "Shaded interval")


def two_bands(c: Context):
    return [
        band(c.gretl, "demo_wave", 0.25, "fill", "Inner band"),
        band(c.gretl, "demo_wave", 0.55, "dash", "Outer band"),
    ]


# Plots are deliberately small, separate callables so a failing
# command can be copied directly into a Python prompt for debugging.
CASES: list[PlotCase] = [
    # ---- Dataset plots: styles, distributions and diagnostics --------
    PlotCase("D01", "dataset", "Time series: two lines",
             lambda c: c.data.plot("demo_wave demo_signal", time_series=True, with_lines=True)),
    PlotCase("D02", "dataset", "Time series: lines and points",
             lambda c: c.data.plot("demo_wave", time_series=True, with_lp=True)),
    PlotCase("D03", "dataset", "Scatter: points",
             lambda c: c.data.plot("demo_signal demo_wave")),
    PlotCase("D04", "dataset", "Scatter: quadratic fitted curve",
             lambda c: c.data.plot("demo_positive demo_wave", fit="quadratic")),
    PlotCase("D05", "dataset", "Vertical impulses",
             lambda c: c.data.plot("demo_pulses", time_series=True, with_impulses=True)),
    PlotCase("D06", "dataset", "Bars / boxes",
             lambda c: c.data.plot("demo_positive", time_series=True, with_boxes=True)),
    PlotCase("D07", "dataset", "Step function",
             lambda c: c.data.plot("demo_steps", time_series=True, with_steps=True)),
    PlotCase("D08", "dataset", "Single shaded band (Bundle)",
             lambda c: c.data.plot("demo_wave", time_series=True, with_lines=True,
                                   band=single_band(c))),
    PlotCase("D09", "dataset", "Two bands: fill and dash",
             lambda c: c.data.plot("demo_wave", time_series=True, with_lines=True,
                                   bands=two_bands(c))),
    PlotCase("D10", "dataset", "Two Y axes",
             lambda c: c.data.plot("demo_wave demo_positive", time_series=True,
                                   with_lines=True, y2axis="demo_positive")),
    PlotCase("D11", "dataset", "Boxplots: three variables",
             lambda c: c.data.boxplot("demo_wave demo_signal demo_positive", notches=True)),
    PlotCase("D12", "dataset", "Scatter panels",
             lambda c: c.data.scatters("demo_wave ; demo_signal demo_positive")),
    PlotCase("D13", "dataset", "Multiple time-series panels",
             lambda c: c.data.tsplots("demo_wave demo_signal demo_positive")),
    PlotCase("D14", "dataset", "Histogram with normal density",
             lambda c: c.data.freq("demo_wave", normal=True, plot="display")),
    PlotCase("D15", "dataset", "Histogram with gamma density",
             lambda c: c.data.freq("demo_positive", gamma=True, plot="display")),
    PlotCase("D16", "dataset", "Kernel density estimate",
             lambda c: c.data.kdplot("demo_wave")),
    PlotCase("D17", "dataset", "Q-Q plot",
             lambda c: c.data.qqplot("demo_wave")),
    PlotCase("D18", "dataset", "Correlation heatmap",
             lambda c: c.data.corr("LRM LRY IBO IDE", plot="display", triangle=True)),
    PlotCase("D19", "dataset", "ACF and PACF",
             lambda c: c.data.corrgm(y="LRM", order=12, plot="display")),
    PlotCase("D20", "dataset", "Cross-correlogram",
             lambda c: c.data.xcorrgm("LRM", "LRY", 12, plot="display")),

    # ---- Model plots: OLS diagnostics and VAR dynamics ---------------
    PlotCase("O01", "model", "OLS: fitted versus actual",
             lambda c: c.ols.plot.fitted_actual()),
    PlotCase("O02", "model", "OLS: residuals",
             lambda c: c.ols.plot.residuals()),
    PlotCase("O03", "model", "OLS: residual correlogram",
             lambda c: c.ols.plot.correlogram(order=12)),
    PlotCase("O04", "model", "OLS: residual Q-Q plot",
             lambda c: c.ols.plot.qqplot()),
    PlotCase("O05", "model", "OLS: residual periodogram",
             lambda c: c.ols.plot.periodogram()),
    PlotCase("O06", "model", "OLS: leverage and influence",
             lambda c: c.ols.leverage(plot="display")),
    PlotCase("V01", "model", "VAR: combined residuals",
             lambda c: c.var.plot.residuals()),
    PlotCase("V02", "model", "VAR: inverse roots",
             lambda c: c.var.plot.inverse_roots()),
    PlotCase("V03", "model", "VAR: impulse response with confidence interval",
             lambda c: c.var.plot.irf(targ=0, shock=1,
                                      periods=16, alpha=0.10)),
    PlotCase("V04", "model", "VAR: FEVD histogram",
             lambda c: c.var.plot.fevd(targ=0, periods=16,
                                      histogram=True)),
    PlotCase("V05", "model", "VAR: FEVD lines",
             lambda c: c.var.plot.fevd(targ=0, periods=16,
                                      histogram=False)),

    # ---- Matrix plots: exercise the separate matrix_command_driver ---
    PlotCase("M01", "matrix", "Matrix: time-series lines",
             lambda c: c.matrix.plot("1 2", time_series=True, with_lines=True)),
    PlotCase("M02", "matrix", "Matrix: lines and points",
             lambda c: c.matrix.plot("1", time_series=True, with_lp=True)),
    PlotCase("M03", "matrix", "Matrix: vertical bars",
             lambda c: c.matrix.plot("3", time_series=True, with_boxes=True)),
    PlotCase("M04", "matrix", "Matrix: shaded confidence band",
             lambda c: c.matrix.plot("1", time_series=True, with_lines=True,
                                     band=c.matrix_band)),
    PlotCase("M05", "matrix", "Matrix: boxplots",
             lambda c: c.matrix.boxplot("1 2 3")),
    PlotCase("M06", "matrix", "Matrix: scatter panels",
             lambda c: c.matrix.scatters("1 ; 2 3")),
]


def extract_figures(result: Any, figure_type: type) -> list[Any]:
    """Accept direct Figure results and (statistics, Figure) tuples."""
    if isinstance(result, figure_type):
        return [result]
    if isinstance(result, (tuple, list)):
        figures = [item for item in result if isinstance(item, figure_type)]
        if figures:
            return figures
    raise TypeError(
        "plot did not return a Matplotlib Figure "
        f"(received {type(result).__name__}: {str(result)[:90]})"
    )


def validate_figure(fig: Any) -> None:
    """Reject an empty figure, not merely a successfully created window."""
    if not fig.axes:
        raise AssertionError("Figure contains no Matplotlib Axes")

    # Lines, bars, filled regions, heatmaps, and plotted collections are
    # distinct Matplotlib artist types. Grid layouts may have blank axes,
    # so at least one axes must contain real graphical data.
    artists = sum(
        len(ax.lines) + len(ax.patches) + len(ax.collections)
        + len(ax.images) + len(ax.containers) + len(ax.artists)
        for ax in fig.axes
    )
    if artists == 0:
        raise AssertionError("Figure contains no plotted data artists")


def cli() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--mode", choices=("demo", "test"), default="demo",
                        help="interactive gallery or headless integration test")
    parser.add_argument("--group", choices=("all", "dataset", "model", "matrix"),
                        default="all", help="run only one family of plots")
    parser.add_argument("--filter", default="", metavar="TEXT",
                        help="case-insensitive substring of case ID/title")
    parser.add_argument("--limit", type=int, default=0,
                        help="run at most N selected cases (0 = all)")
    parser.add_argument("--save-dir", type=Path,
                        help="optionally write PNG previews to this directory")
    parser.add_argument("--list", action="store_true",
                        help="list cases without importing gretl4py")
    parser.add_argument("--verbose", action="store_true",
                        help="print full traceback for failed plots")
    return parser.parse_args()


def selected_cases(args: argparse.Namespace) -> list[PlotCase]:
    selected = [case for case in CASES
                if (args.group == "all" or case.group == args.group)
                and (not args.filter or args.filter.casefold() in
                     f"{case.id} {case.title}".casefold())]
    return selected[:args.limit] if args.limit > 0 else selected


def main() -> int:
    args = cli()
    cases = selected_cases(args)

    if args.list:
        for case in cases:
            print(f"{case.id}  {case.group:<7}  {case.title}")
        print(f"\n{len(cases)} selected / {len(CASES)} total")
        return 0
    if not cases:
        print("No matching plot cases.", file=sys.stderr)
        return 2

    # Select a non-interactive backend BEFORE importing pyplot or gretl.
    if args.mode == "test":
        import os
        os.environ["MPLBACKEND"] = "Agg"
        import matplotlib
        matplotlib.use("Agg", force=True)

    import matplotlib.pyplot as plt
    from matplotlib.figure import Figure
    import gretl

    if args.save_dir is not None:
        args.save_dir.mkdir(parents=True, exist_ok=True)

    # Disable automatic display even in demo mode: figures will be
    # displayed sequentially, preventing dozens of simultaneous windows.
    old_auto_show = gretl.get_plot_auto_show()
    gretl.set_plot_auto_show(False)
    context = Context(gretl)
    passed = 0
    failed: list[tuple[str, str]] = []

    try:
        print(f"Mode: {args.mode}; cases: {len(cases)}; "
              f"Matplotlib backend: {plt.get_backend()}")

        for index, case in enumerate(cases, 1):
            print(f"[{index:02d}/{len(cases):02d}] {case.id} {case.title} ... ",
                  end="", flush=True)
            try:
                result = case.make(context)
                figures = extract_figures(result, Figure)
                for fig in figures:
                    validate_figure(fig)
                    # Explicit rendering also catches many lazy Matplotlib errors.
                    fig.canvas.draw()

                if args.save_dir is not None:
                    stem = re.sub(r"[^a-z0-9]+", "_", case.title.lower()).strip("_")
                    for k, fig in enumerate(figures, 1):
                        suffix = f"_{k}" if len(figures) > 1 else ""
                        dest = args.save_dir / f"{case.id}_{stem}{suffix}.png"
                        fig.savefig(dest, dpi=150, bbox_inches="tight")

                passed += 1
                print(f"PASS ({len(figures)} figure(s), "
                      f"{sum(len(fig.axes) for fig in figures)} axes)")

                if args.mode == "demo":
                    # The window is closed by the user before the next plot.
                    plt.show(block=True)
            except Exception as exc:
                failed.append((case.id, f"{type(exc).__name__}: {exc}"))
                print(f"FAIL: {type(exc).__name__}: {exc}")
                if args.verbose:
                    traceback.print_exc()
            finally:
                # Close everything, including figures created before a
                # plot failed. This matters in long headless test runs.
                plt.close("all")
    finally:
        gretl.set_plot_auto_show(old_auto_show)

    print("\n" + "-" * 62)
    print(f"Total: {len(cases)} | PASS: {passed} | FAIL: {len(failed)}")
    for name, reason in failed:
        print(f"  {name}: {reason}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
