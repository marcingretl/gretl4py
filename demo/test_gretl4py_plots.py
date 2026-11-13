#!/usr/bin/env python3
"""Exercise gretl4py gnuplot options and save failing gnuplot scripts.

Run: python3 test_gretl4py_plots_factorized.py

The non-interactive Agg backend avoids opening a window for every test.
The tests check that plotting returns a Matplotlib Figure and save PNGs
for manual visual inspection; they do not assert visual correctness.
"""

import os

# Set the backend before importing gretl or pyplot. The gretl4py plotting
# wrapper calls plt.show(block=True), which should not open GUI windows here.
#os.environ["MPLBACKEND"] = "Agg"

import csv
import re
import traceback
from dataclasses import dataclass, field
from pathlib import Path

import gretl
from matplotlib.figure import Figure
import matplotlib.pyplot as plt

OUTPUT_DIR = Path("/tmp/plottests")
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
PNG_DIR = OUTPUT_DIR / "png"
PNG_DIR.mkdir(parents=True, exist_ok=True)


@dataclass(frozen=True)
class Case:
    name: str
    varlist: str
    options: dict = field(default_factory=dict)
    # Tests that need user-provided gretl objects or special data remain
    # explicit rather than silently pretending they were exercised.
    skip_reason: str = ""


# denmark.gdt provides LRM, LRY and IBO. We use LRM/LRY for regular
# plots, and the third series for the controlled-plot variant.
CASES = [
    Case("default_scatter", "LRM LRY"),
    Case("default_time_series", "LRM LRY", {"time_series": True}),
    Case("with_lines", "LRM LRY", {"with_lines": True}),
    Case("with_lines_varspec", "LRM LRY", {"with_lines": "LRM"}),
    Case("with_lp", "LRM LRY", {"with_lp": True}),
    Case("with_lp_varspec", "LRM LRY", {"with_lp": "LRM"}),
    Case("with_impulses", "LRM LRY", {"with_impulses": True}),
    Case("with_impulses_varspec", "LRM LRY", {"with_impulses": "LRM"}),
    Case("with_boxes", "LRM LRY", {"with_boxes": True}),
    Case("with_boxes_varspec", "LRM LRY", {"with_boxes": "LRM"}),
    Case("with_steps", "LRM LRY", {"with_steps": True}),
    Case("with_steps_varspec", "LRM LRY", {"with_steps": "LRM"}),
    Case("time_series", "LRM LRY", {"time_series": True}),
    Case("single_yaxis", "LRM LRY IBO", {"single_yaxis": True}),
    Case("y2axis", "LRM LRY IBO", {"y2axis": "LRY"}),
    Case("ylogscale_default", "LRM LRY", {"ylogscale": True}),
    Case("ylogscale_base2", "LRM LRY", {"ylogscale": 2}),
    Case("fit_linear", "LRM LRY", {"fit": "linear"}),
    Case("fit_none", "LRM LRY", {"fit": "none"}),
    Case("fit_inverse", "LRM LRY", {"fit": "inverse"}),
    Case("fit_quadratic", "LRM LRY", {"fit": "quadratic"}),
    Case("fit_cubic", "LRM LRY", {"fit": "cubic"}),
    Case("fit_semilog", "LRM LRY", {"fit": "semilog"}),
    Case("fit_linlog", "LRM LRY", {"fit": "linlog"}),
    Case("fit_loess", "LRM LRY", {"fit": "loess"}),
    Case("font", "LRM LRY", {"font": "serif,12"}),
    Case("control", "LRM LRY IBO", {"control": True}),
    # The discrete factor series is generated in main() before testing.
    Case("factorized", "LRM LRY plotfactor", {"factorized": True}),
    Case("factorized_varspec", "LRM LRY plotfactor", {"factorized": "LRM"}),
    Case("band", "LRM", {"time_series": True, "band": "b1"},
         "Requires a named gretl bundle b1 with center and width"),
    Case("bands", "LRM", {"time_series": True, "bands": "bb"},
         "Requires a named gretl array of bundles bb"),
    Case("matrix", "", {"matrix": "M"},
         "Requires a named gretl matrix M"),
    Case("outbuf", "LRM LRY", {"outbuf": "plotbuf"},
         "Writes a gretl string, not a displayed plot"),
    Case("output", "LRM LRY", {"output": "display"},
         "Output is exercised by the failure-recovery path"),
    Case("input", "", skip_reason="Disabled as requested"),
    Case("inbuf", "", skip_reason="Disabled as requested"),
]


def filename_for(name: str) -> Path:
    """Use a safe, deterministic filename for each test."""
    return OUTPUT_DIR / (re.sub(r"[^A-Za-z0-9_-]+", "_", name) + ".gp")


def run_case(dataset, case: Case) -> dict:
    record = {"test": case.name, "status": "", "error": "", "gp_file": "", "fallback_error": "", "png_file": ""}

    if case.skip_reason:
        record["status"] = "SKIP"
        record["error"] = case.skip_reason
        return record

    print(f"[{case.name}] plot: {case.varlist!r}, options={case.options}", flush=True)
    fig = None
    try:
        fig = dataset.plot(case.varlist, **case.options)
        if not isinstance(fig, Figure):
            raise TypeError(f"Expected matplotlib.figure.Figure, got {type(fig).__name__}")
        # Save the rendered figure for visual inspection.
        png_file = PNG_DIR / (filename_for(case.name).stem + ".png")
        fig.savefig(png_file, dpi=150, bbox_inches="tight")
        record["png_file"] = str(png_file)
        record["status"] = "OK"
    except Exception as exc:
        record["status"] = "FAIL"
        record["error"] = f"{type(exc).__name__}: {exc}"
        print(f"  FAILED: {record['error']}", flush=True)
        traceback.print_exc()

        # Repeat the same plot command with only the output destination
        # changed. This captures the original gnuplot script without
        # invoking the matplotlib translator/rendering path.
        gp_file = filename_for(case.name)
        fallback_options = dict(case.options)
        fallback_options["output"] = str(gp_file)
        try:
            dataset.plot(case.varlist, **fallback_options)
            if gp_file.is_file() and gp_file.stat().st_size:
                record["gp_file"] = str(gp_file)
                print(f"  Saved gnuplot script: {gp_file}", flush=True)
            else:
                record["fallback_error"] = "No nonempty .gp file was produced"
                print(f"  FALLBACK FAILED: {record['fallback_error']}", flush=True)
        except Exception as fallback_exc:
            record["fallback_error"] = f"{type(fallback_exc).__name__}: {fallback_exc}"
            print(f"  FALLBACK FAILED: {record['fallback_error']}", flush=True)
            traceback.print_exc()
    finally:
        # Avoid accumulating Matplotlib figures during a long test run.
        # The returned Figure is no longer needed after validation.
        if isinstance(fig, Figure):
            plt.close(fig)
        plt.close("all")

    return record


def main() -> int:
    dataset = gretl.get_data("denmark")

    # Create a discrete 0/1 factor using the dataset expression API.
    # This series is used only by the factorized plotting cases.
    dataset.new_series("plotfactor = (obs % 2 == 0)")

    records = [run_case(dataset, case) for case in CASES]
    report_path = OUTPUT_DIR / "report.csv"
    with report_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=["test", "status", "error", "gp_file", "fallback_error", "png_file"])
        writer.writeheader()
        writer.writerows(records)

    counts = {status: sum(row["status"] == status for row in records)
              for status in ("OK", "FAIL", "SKIP")}
    print(f"\nRESULT: {counts['OK']} OK, {counts['FAIL']} FAIL, {counts['SKIP']} SKIP")
    print(f"Report: {report_path}")
    print(f"Failed plot scripts: {OUTPUT_DIR}/*.gp")
    print(f"Rendered figures: {PNG_DIR}/*.png")
    return 1 if counts["FAIL"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
