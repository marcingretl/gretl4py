#!/usr/bin/env python3
"""Run approved gretl package examples in isolated processes.

Each sample is a separate interpreter to avoid leaking gretl/libgretl state.
Stop on the first native or Python error, even when run_script() returns normally.
The --approved flag authorizes running the selected third-party examples.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys

from audit_config import AUDIT_ROOT, CONTRIBUTED_DIR, RESULTS_ROOT
from probe_package_sample import (
    find_downloaded_contributed, native_error_reason, resolve_sample_name,
)

PROBE = Path(__file__).resolve().with_name("probe_package_sample.py")
SAMPLE_MARKER = re.compile(r"(?mi)^\[EXAMPLE\] (.+\.inp)$")
FIGURE_MARKER = re.compile(r"(?m)^\[RESULT\] Active Matplotlib figures: (\d+)\s*$")
EXAMPLES_DIR_MARKER = re.compile(r"(?m)^\[EXAMPLES\] (.+)$")
# Direct, literal hansl `open file.bin` statements require native gretl
# database support. Do not match references in comments or arbitrary text.
BIN_OPEN_MARKER = re.compile(
    r"^\s*open\s+(?:[\"']([^\"']+\.bin)[\"']|([^\s#;]+\.bin))(?=\s|$|;)",
    re.IGNORECASE,
)



@dataclass
class CommandResult:
    returncode: int
    output: str
    error: str | None


def run_command(command: list[str], timeout: int) -> CommandResult:
    """Capture both Python and native libgretl output with a finite deadline."""
    try:
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, errors="replace", timeout=timeout)
        native_error = native_error_reason(result.stdout)
        error = native_error or (f"process exited with status {result.returncode}"
                                 if result.returncode else None)
        return CommandResult(result.returncode, result.stdout, error)
    except subprocess.TimeoutExpired as exc:
        # A timed-out child is terminated by subprocess.run().
        output = exc.stdout or b""
        if isinstance(output, bytes):
            output = output.decode(errors="replace")
        return CommandResult(124, output, f"timeout after {timeout} seconds")


def candidates_from_csv(filename: Path, contributed_only: bool) -> list[str]:
    """Find plotting candidates from the scanner's existing catalog."""
    if not filename.is_file():
        raise FileNotFoundError(f"Missing {filename}; run 'python package_plot_audit.py scan'")
    candidates: list[str] = []
    with filename.open(newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            if row.get("status") == "no_plot_detected":
                continue
            name = (row.get("package") or "").strip()
            if not name or name in candidates:
                continue
            if contributed_only and find_downloaded_contributed(name, CONTRIBUTED_DIR) is None:
                continue
            candidates.append(name)
    return candidates


def classify(command: CommandResult) -> tuple[str, str]:
    """Do not mistake native diagnostic text for a successful execution."""
    if command.error:
        return "FAIL", command.error
    found = FIGURE_MARKER.search(command.output)
    if found is None:
        return "FAIL", "probe returned no figure-count marker"
    count = int(found.group(1))
    if count == 0:
        return "NO_FIGURES", "sample produced no active Matplotlib figure"
    return "PASS", f"{count} figure(s)"



def bin_database_reference(sample: Path) -> tuple[int, str] | None:
    """Return line and database name for a direct `open *.bin` statement.

    This is deliberately conservative: computed file names or nested `run`
    scripts cannot be resolved reliably without executing third-party hansl.
    """
    with sample.open(encoding="utf-8-sig", errors="replace") as fh:
        for line_number, line in enumerate(fh, 1):
            # Comments cannot trigger database loading.
            if line.lstrip().startswith("#"):
                continue
            match = BIN_OPEN_MARKER.match(line)
            if match:
                return line_number, match.group(1) or match.group(2)
    return None


def installed_sample_path(preparation_output: str, sample: str) -> Path:
    """Resolve a discovered sample against the installed examples directory."""
    found = EXAMPLES_DIR_MARKER.search(preparation_output)
    if not found:
        raise ValueError("Prepare output lacks [EXAMPLES] directory marker")
    base = Path(found.group(1).strip()).resolve()
    chosen = (base / sample).resolve()
    # Never inspect a path outside examples/, including escaping symlinks.
    if not chosen.is_relative_to(base):
        raise ValueError(f"Sample escapes examples directory: {sample}")
    return chosen

def safe_stem(value: str) -> str:
    """Keep package/sample names out of report file paths."""
    return re.sub(r"[^A-Za-z0-9._-]", "_", value).strip(".") or "unnamed"


def record_result(report: Path, row: dict[str, str]) -> None:
    """Persist every completed run immediately, including the first failure."""
    fields = ["package", "sample", "status", "reason", "log"]
    exists = report.is_file()
    with report.open("a", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=fields)
        if not exists:
            writer.writeheader()
        writer.writerow(row)


def run_batch(args: argparse.Namespace) -> int:
    """Prepare each package, execute its examples, stop on first failure."""
    packages = list(dict.fromkeys(args.packages))
    if args.candidates:
        packages += [p for p in candidates_from_csv(args.catalog, args.contributed_only)
                     if p not in packages]
    # Apply the offset to the complete, deduplicated candidate list before
    # limiting the batch, so subsequent batches can skip already-tested packages.
    packages = packages[args.offset:]
    if args.limit is not None:
        packages = packages[:args.limit]
    if not packages:
        raise ValueError("No packages selected. Specify a name or --candidates")

    out = args.out.resolve()
    (out / "logs").mkdir(parents=True, exist_ok=True)
    report = out / "batch_results.csv"
    # Keep separate reports for separate invocations; do not mix stale results.
    report.unlink(missing_ok=True)

    totals = {"PASS": 0, "FAIL": 0, "NO_FIGURES": 0, "NO_SAMPLES": 0, "SKIPPED_BIN": 0}

    def save(pkg: str, sample: str, result: CommandResult, status: str, reason: str) -> None:
        key = f"{safe_stem(pkg)}__{safe_stem(sample)}"
        log = out / "logs" / f"{key}.log"
        log.write_text(result.output, encoding="utf-8")
        record_result(report, {"package": pkg, "sample": sample, "status": status,
                               "reason": reason, "log": str(log)})
        totals[status] += 1
        print(f"[{status}] {pkg} / {sample}: {reason}")
        if status == "FAIL":
            print(f"        Full diagnostic log: {log}")

    for pkg in packages:
        # Installing or including the package is performed in a dedicated
        # subprocess, using only the already-downloaded local cache.
        print(f"[PREPARE] {pkg}", flush=True)
        prepare = run_command([
            sys.executable, "-u", str(PROBE), pkg, "--source", args.source,
            "--prepare", "--approved", "--_worker",
        ], args.timeout)
        if prepare.error:
            save(pkg, "<prepare>", prepare, "FAIL", prepare.error)
            if not args.continue_on_error:
                break
            continue

        samples = SAMPLE_MARKER.findall(prepare.output)
        if args.samples:
            # Resolve selected names against recursive example paths. Short
            # names are allowed only when they identify one example uniquely.
            try:
                samples = [resolve_sample_name(samples, name) for name in args.samples]
            except (ValueError, FileNotFoundError) as exc:
                failure = CommandResult(1, prepare.output, str(exc))
                save(pkg, "<selection>", failure, "FAIL", failure.error or "unknown error")
                if not args.continue_on_error:
                    break
                continue
        elif not samples:
            # An addon with no installed .inp files is not automatically a
            # failed translator test: there is simply nothing to execute.
            save(pkg, "<none>", prepare, "NO_SAMPLES", "no installed .inp examples")
            continue

        print(f"[SAMPLES] {pkg}: {', '.join(s or '<default>' for s in samples)}", flush=True)
        for sample in samples:
            chosen = sample or "<default>"
            if args.skip_bin and sample:
                # Skip unsupported .bin database inputs *before* running hansl.
                # The skipped case remains visible in both CSV and diagnostic log.
                try:
                    reference = bin_database_reference(
                        installed_sample_path(prepare.output, sample)
                    )
                except (OSError, ValueError) as exc:
                    result = CommandResult(1, f"[PREFLIGHT] {exc}\n", str(exc))
                    save(pkg, chosen, result, "FAIL", str(exc))
                    if not args.continue_on_error:
                        break
                    continue
                if reference is not None:
                    line_number, filename = reference
                    reason = f"requires .bin database: open {filename} (line {line_number})"
                    result = CommandResult(0, f"[SKIPPED_BIN] {chosen}: {reason}\n", None)
                    save(pkg, chosen, result, "SKIPPED_BIN", reason)
                    continue

            command = [sys.executable, "-u", str(PROBE), pkg, "--source", args.source,
                       "--approved", "--_worker", "--out",
                       str(out / "images" / safe_stem(pkg) / safe_stem(chosen))]
            if sample is not None:
                command += ["--sample", sample]
            result = run_command(command, args.timeout)
            status, reason = classify(result)
            save(pkg, chosen, result, status, reason)
            if status == "FAIL" and not args.continue_on_error:
                break
        if totals["FAIL"] and not args.continue_on_error:
            break

    print(f"[SUMMARY] PASS={totals['PASS']}, NO_FIGURES={totals['NO_FIGURES']}, "
          f"NO_SAMPLES={totals['NO_SAMPLES']}, SKIPPED_BIN={totals['SKIPPED_BIN']}, "
          f"FAIL={totals['FAIL']} | {report}")
    return 1 if totals["FAIL"] else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packages", nargs="*", help="Names of packages to test")
    parser.add_argument("--candidates", action="store_true",
                        help="Also run packages selected by the static scan")
    parser.add_argument("--catalog", type=Path, default=AUDIT_ROOT / "plot_packages.csv")
    parser.add_argument("--contributed-only", action="store_true",
                        help="When using --candidates, omit packages without a downloaded contributed archive")
    parser.add_argument("--source", choices=("auto", "addon", "contributed"), default="auto")
    parser.add_argument("--samples", nargs="+", metavar="NAME",
                        help="Run only these named examples (for selected packages)")
    parser.add_argument("--offset", type=int, default=0,
                        help="Skip the first N selected packages (default: 0)")
    parser.add_argument("--limit", type=int, default=None,
                        help="Run at most N packages after applying --offset")
    parser.add_argument("--timeout", type=int, default=300,
                        help="Maximum seconds per prepare/sample (default: 300)")
    parser.add_argument("--continue-on-error", action="store_true",
                        help="Continue after failures; default is to stop on the first error")
    parser.add_argument("--skip-bin", action="store_true",
                        help="Skip examples with a literal hansl 'open *.bin' command")
    parser.add_argument("--out", type=Path, default=RESULTS_ROOT / "batch")
    parser.add_argument("--approved", action="store_true",
                        help="Confirm that you have reviewed and approved all selected examples")
    args = parser.parse_args()
    if not args.approved:
        parser.error("Refusing to run third-party code without --approved")
    if args.timeout <= 0 or (args.limit is not None and args.limit <= 0):
        parser.error("--timeout and --limit must be positive")
    if args.offset < 0:
        parser.error("--offset must not be negative")
    try:
        return run_batch(args)
    except (ValueError, FileNotFoundError, OSError) as exc:
        print(f"[FAIL] {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
