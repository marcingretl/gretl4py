#!/usr/bin/env python3
"""Manually approved gretl4py integration probe for one gretl package.

Do NOT run third-party samples without reviewing their code and dependencies.
Each probe should be invoked in a fresh Python process to isolate gretl state.
Contributed packages are installed only from previously downloaded local files.
"""

from __future__ import annotations
import argparse
import csv
from dataclasses import dataclass
from contextlib import chdir
import os
from pathlib import Path
import re
import subprocess
import sys

from audit_config import CONTRIBUTED_DIR, RESULTS_ROOT

# A custom fixture suite is data for this auditor, not an independent runner.
CUSTOM_TEST_ROOT = Path(__file__).resolve().parent / "tests"


def find_downloaded_contributed(name: str, directory: Path = CONTRIBUTED_DIR) -> Path | None:
    """Find a cached .gfn or .zip package without downloading anything."""
    if not name or name in {".", ".."} or Path(name).name != name or "/" in name or "\\" in name:
        raise ValueError(f"Invalid package name: {name!r}")

    # Prefer the directly installable .gfn when both formats are present.
    for extension in (".gfn", ".zip"):
        candidate = directory / (name + extension)
        if candidate.is_file():
            return candidate.resolve()

    # The catalog's filename capitalization may differ from the package name.
    if directory.is_dir():
        matches = [
            path for path in directory.iterdir()
            if path.is_file()
            and path.suffix.lower() in {".gfn", ".zip"}
            and path.stem.casefold() == name.casefold()
        ]
        if matches:
            matches.sort(key=lambda path: (path.suffix.lower() != ".gfn", path.name))
            return matches[0].resolve()
    return None


def load_package(gretl, name: str, source: str = "auto",
                 contributed_dir: Path = CONTRIBUTED_DIR):
    """Include addons normally; install missing contributed packages from cache."""
    if source not in {"auto", "addon", "contributed"}:
        raise ValueError(f"Unsupported package source: {source}")

    # Official addons continue to use the existing gretl installation.
    if source == "addon":
        print(f"[ADDON] Including {name}")
        return gretl.include(name)

    cached = find_downloaded_contributed(name, contributed_dir)
    if source == "auto" and cached is None:
        # No downloaded contributed package: use the existing addon route.
        # This also supports packages that are already installed locally.
        print(f"[INCLUDE] {name} (no contributed archive in {contributed_dir})")
        return gretl.include(name)

    # For contributed packages, reuse existing installs first.
    try:
        return gretl.include(name)
    except FileNotFoundError:
        if cached is None:
            raise FileNotFoundError(
                f"Package '{name}' is not installed and no cached .gfn/.zip "
                f"was found in {contributed_dir}. Run the fetch step first."
            ) from None
        print(f"[INSTALL] Package '{name}' not found. Installing cached {cached}")
        gretl.pkg("install", str(cached), local=True)
        return gretl.include(name)



def installed_examples_dir(gretl, name: str, override: Path | None = None) -> Path:
    """Locate examples in the installed Python-bundled gretl package."""
    if override is not None:
        return override.expanduser().resolve()

    # gretl4py installs contributed packages under gretl/functions/<name>/.
    return Path(gretl.__file__).resolve().parent / "functions" / name / "examples"


def list_sample_scripts(directory: Path) -> list[Path]:
    """List installed hansl examples recursively, including addon subdirectories."""
    if not directory.is_dir():
        return []
    return sorted(
        (p for p in directory.rglob("*") if p.is_file() and p.suffix.lower() == ".inp"),
        key=lambda p: (p.relative_to(directory).as_posix().casefold(),
                       p.relative_to(directory).as_posix()),
    )


def sample_relative_name(directory: Path, sample: Path) -> str:
    """Keep subdirectory names when displaying or selecting a script."""
    return sample.relative_to(directory).as_posix()


def resolve_sample_name(available: list[str], name: str) -> str:
    """Resolve an explicit relative path or an unambiguous example basename.

    Reject absolute paths, traversal and Windows separators; never allow a
    sample selection to escape the installed package's examples directory.
    """
    if (not name or name.startswith("/") or "\\" in name or ":" in name or
            any(part in {"", ".", ".."} for part in name.split("/"))):
        raise ValueError(f"Invalid sample filename: {name!r}")

    requested = name if name.lower().endswith(".inp") else name + ".inp"
    exact = [item for item in available if item.casefold() == requested.casefold()]
    if exact:
        return exact[0]

    # The short form is convenient unless different folders contain the
    # same filename. In that case the caller must pass the relative path.
    if "/" not in requested:
        matches = [item for item in available if item.rsplit("/", 1)[-1].casefold()
                   == requested.casefold()]
        if len(matches) == 1:
            return matches[0]
        if len(matches) > 1:
            raise ValueError(
                f"Ambiguous sample {requested!r}: {', '.join(matches)}. "
                "Use a path relative to examples/."
            )

    raise FileNotFoundError(
        f"Sample {requested!r} not found; available: {', '.join(available) or 'none'}"
    )


def choose_sample_script(directory: Path, name: str | None = None) -> Path | None:
    """Select a reviewed example, preserving paths inside nested directories.

    If no name is given, prefer a conventional top-level Package_sample.inp;
    only auto-select a noncanonical example if exactly one exists.
    """
    samples = list_sample_scripts(directory)
    available = [sample_relative_name(directory, p) for p in samples]
    if name is not None:
        selected_name = resolve_sample_name(available, name)
        chosen = directory / selected_name
    else:
        canonical = directory.parent.name + "_sample.inp"
        if canonical in available:
            chosen = directory / canonical
        elif len(samples) == 1:
            chosen = samples[0]
        elif len(samples) > 1:
            raise ValueError(
                f"Package has multiple examples ({', '.join(available)}). "
                "Choose one using --sample PATH, or inspect them via --list-samples."
            )
        else:
            return None

    # A listed file may itself be a symlink; keep it inside examples/.
    chosen = chosen.resolve()
    if not chosen.is_relative_to(directory.resolve()):
        raise ValueError(f"Sample points outside its examples directory: {chosen}")
    return chosen


# Some nested package examples open data using paths relative to the package
# root, such as `open examples/Usage_examples/data.gdt`.
PACKAGE_RELATIVE_OPEN = re.compile(
    r"^\s*open\s+(?:[\"']((?:\./)?examples/[^\"']+)[\"']|((?:\./)?examples/[^\s#;]+))",
    re.IGNORECASE,
)


def sample_working_directory(script: Path, examples_dir: Path) -> Path | None:
    """Use the package root only when a sample opens an existing `examples/` file.

    Leave other samples untouched: they may rely on a different cwd, and
    changing the entire batch's cwd would invalidate previously passing tests.
    """
    package_dir = examples_dir.resolve().parent
    with script.open(encoding="utf-8-sig", errors="replace") as source:
        for line in source:
            if line.lstrip().startswith("#"):
                continue
            match = PACKAGE_RELATIVE_OPEN.match(line)
            if match is None:
                continue
            candidate = (package_dir / (match.group(1) or match.group(2))).resolve()
            # Do not follow sample-supplied relative paths outside the package.
            if candidate.is_relative_to(package_dir) and candidate.is_file():
                return package_dir
    return None


@dataclass(frozen=True)
class CustomCase:
    """One reviewed hansl fixture and its installed package dependency."""
    name: str
    package: str
    script: Path
    workdir: Path


def load_custom_cases(suite: str, root: Path = CUSTOM_TEST_ROOT) -> list[CustomCase]:
    """Load a reviewed suite; reject traversal and scripts outside its directory."""
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", suite):
        raise ValueError(f"Invalid custom suite name: {suite!r}")
    base = root.resolve()
    workdir = (base / suite).resolve()
    if not workdir.is_relative_to(base):
        raise ValueError(f"Custom suite escapes test root: {suite}")
    manifest = workdir / "manifest.csv"
    if not manifest.is_file():
        raise FileNotFoundError(f"Custom suite manifest not found: {manifest}")
    cases: list[CustomCase] = []
    names: set[str] = set()
    with manifest.open(newline="", encoding="utf-8-sig") as fh:
        reader = csv.DictReader(fh)
        if reader.fieldnames != ["case", "package", "script"]:
            raise ValueError(f"Invalid custom suite manifest columns: {manifest}")
        for row in reader:
            if any(row.get(key) is None for key in ("case", "package", "script")):
                raise ValueError(f"Incomplete custom fixture row in {manifest}")
            name, package, relative = (row[key].strip() for key in ("case", "package", "script"))
            if (not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", name)
                    or name in names):
                raise ValueError(f"Invalid or duplicate custom case: {name!r}")
            if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]*", package):
                raise ValueError(f"Invalid custom package name: {package!r}")
            # All fixture scripts live under samples/, never outside the suite.
            if (not relative.startswith("samples/") or "\\" in relative
                    or any(part in {"", ".", ".."} for part in relative.split("/"))):
                raise ValueError(f"Invalid custom fixture path: {relative!r}")
            script = (workdir / relative).resolve()
            if (not script.is_relative_to(workdir / "samples")
                    or not script.is_file() or script.suffix != ".inp"):
                raise ValueError(f"Missing or unsafe custom fixture: {relative!r}")
            cases.append(CustomCase(name, package, script, workdir))
            names.add(name)
    if not cases:
        raise ValueError(f"Custom suite contains no cases: {suite}")
    return cases


def execute_with_workdir(gretl, script: Path, workdir: Path) -> None:
    """Synchronize Python's cwd and libgretl's $workdir for relative data paths."""
    # A script may change libgretl's workdir; every example runs in its own
    # process so no restoration of libgretl state is necessary afterwards.
    location = str(workdir.resolve()).replace('"', '\\"')
    with chdir(workdir):
        gretl.run_hansl(f'set workdir "{location}"')
        gretl.run_script(str(script))


# libgretl may print a native error without raising a Python exception.
# Probe invocations therefore run in a supervised child process, whose
# combined output can be examined before reporting success.
NATIVE_ERROR_PATTERNS = (
    ("gretl_spawn", re.compile(r"(?m)^\s*gretl_spawn:\s*status\s*=\s*([1-9]\d*)\b")),
    ("gnuplot parser", re.compile(r"(?m)(?:^|['\\n])\s*error:\d+:")),
    ("libgretl", re.compile(r"(?mi)^\s*Error in [A-Za-z_][\w-]*\s*:")),
    ("failed command", re.compile(r"(?mi)^\s*Failed command:")),
)


def native_error_reason(output: str) -> str | None:
    """Identify printed native failures that do not propagate as Python exceptions."""
    for name, pattern in NATIVE_ERROR_PATTERNS:
        match = pattern.search(output)
        if match:
            if name == "gretl_spawn":
                # Preserve the first parser error in the CSV summary.
                first_error = re.search(r"error:\d+:[^\r\n']*", output)
                if first_error:
                    return f"gretl_spawn status={match.group(1)}; {first_error.group(0).strip()}"
            return f"{name}: {match.group(0).strip()}"
    return None


def supervise_probe(argv: list[str]) -> int:
    """Isolate one probe and turn printed native failures into a nonzero exit."""
    cmd = [sys.executable, "-u", str(Path(__file__).resolve()), *argv, "--_worker"]
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, errors="replace")
    sys.stdout.write(result.stdout)
    sys.stdout.flush()
    reason = native_error_reason(result.stdout)
    if reason:
        print(f"[FAIL] Native gretl/gptranslate error detected: {reason}", file=sys.stderr)
        return 1
    return result.returncode

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", help="Package name, e.g. SVAR or PairPlot")
    parser.add_argument("--source", choices=("auto", "addon", "contributed"), default="auto",
                        help="Auto-detect from downloaded contributed files (default)")
    parser.add_argument("--custom", metavar="SUITE",
                        help="Run a reviewed fixture from tests/SUITE/")
    parser.add_argument("--case", metavar="CASE",
                        help="Case name from the custom suite's manifest.csv")
    parser.add_argument("--sample", metavar="NAME",
                        help="Run an installed .inp example, optionally in a subdirectory (e.g. Traditional/BlQuah.inp)")
    parser.add_argument("--list-samples", action="store_true",
                        help="List installed .inp examples without executing package code")
    parser.add_argument("--examples-dir", type=Path, default=None,
                        help="Override the installed examples directory")
    parser.add_argument("--approved", action="store_true",
                        help="Explicitly approve executing this package's bundled sample")
    parser.add_argument("--out", type=Path, default=RESULTS_ROOT,
                        help="Directory for rendered PNG files")
    parser.add_argument("--show", action="store_true", help="Open figures interactively")
    parser.add_argument("--prepare", action="store_true",
                        help="Load/install from local cache and list available examples, without executing them")
    parser.add_argument("--_worker", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()
    # Keep output paths stable if an example temporarily changes cwd.
    args.out = args.out.expanduser().resolve()
    if not args.approved and not args.list_samples:
        parser.error("Refusing to execute third-party package code without --approved")
    if args.prepare and args.list_samples:
        parser.error("Use either --prepare or --list-samples")
    if bool(args.custom) != bool(args.case):
        parser.error("--custom and --case must be specified together")
    if args.custom and (args.sample or args.prepare or args.list_samples or args.examples_dir):
        parser.error("--custom cannot be mixed with installed-sample options")

    # The parent examines native diagnostic output. The worker executes code.
    if not args.list_samples and not args._worker:
        return supervise_probe(sys.argv[1:])

    if args.list_samples:
        # Listing samples is intentionally read-only; it does not include or
        # install third-party packages.
        import gretl
        directory = installed_examples_dir(gretl, args.package, args.examples_dir)
        samples = list_sample_scripts(directory)
        print(f"[EXAMPLES] {directory}")
        if not samples:
            print("No installed .inp samples found. Install the package first.")
            return 1
        for sample in samples:
            print(f"  {sample_relative_name(directory, sample)}")
        return 0
    if args.prepare:
        # Install only from the cached local archive; do not run a sample.
        import gretl
        print(f"[LOAD] {args.package}")
        load_package(gretl, args.package, source=args.source)
        directory = installed_examples_dir(gretl, args.package, args.examples_dir)
        print(f"[EXAMPLES] {directory}")
        for sample in list_sample_scripts(directory):
            print(f"[EXAMPLE] {sample_relative_name(directory, sample)}")
        return 0

    if not args.show:
        # Set this before importing matplotlib or gretl.
        os.environ.setdefault("MPLBACKEND", "Agg")

    import matplotlib.pyplot as plt
    import gretl

    args.out.mkdir(parents=True, exist_ok=True)
    old_show = gretl.get_plot_auto_show()
    gretl.set_plot_auto_show(args.show)
    print(f"[LOAD] {args.package}")
    try:
        pkg = load_package(gretl, args.package, source=args.source)
        if pkg is None:
            raise RuntimeError("gretl.include() did not return a package")

        if args.custom:
            # Custom fixtures use the same package loader and output handling
            # as installed examples, but never depend on upstream .inp files.
            cases = load_custom_cases(args.custom)
            fixture = next((item for item in cases if item.name == args.case), None)
            if fixture is None or fixture.package != args.package:
                raise ValueError(f"Case {args.case!r} is not registered for {args.package!r}")
            print(f"[SAMPLE] Execute reviewed custom fixture: {args.custom}/{fixture.name}")
            print(f"[WORKDIR] {fixture.workdir}", flush=True)
            execute_with_workdir(gretl, fixture.script, fixture.workdir)
        else:
            # Addons and contributed packages share recursive example discovery.
            # The package-loading mechanism is intentionally unchanged.
            directory = installed_examples_dir(gretl, args.package, args.examples_dir)
            script = choose_sample_script(directory, args.sample)
            if script is None:
                raise FileNotFoundError(
                    f"No installed .inp examples found in {directory}. "
                    "Use --examples-dir if the package stores examples elsewhere."
                )

            relative = sample_relative_name(directory.resolve(), script)
            print(f"[SAMPLE] Execute approved example: {relative}")
            if script.parent == directory.resolve() and script.name == args.package + "_sample.inp":
                # Preserve the original run_sample() path for conventional samples.
                pkg.run_sample()
            else:
                # run_script() accepts an absolute path and preserves the
                # file context, unlike raw text with run_hansl().
                # A few examples also require package-root-relative data paths.
                workdir = sample_working_directory(script, directory)
                if workdir is not None:
                    print(f"[CWD] Package-root-relative example data: {workdir}", flush=True)
                    # Both workdir mechanisms must agree for gretl's `open`.
                    execute_with_workdir(gretl, script, workdir)
                else:
                    # Leave unrelated examples' existing cwd behavior untouched.
                    gretl.run_script(str(script))

        numbers = plt.get_fignums()
        print(f"[RESULT] Active Matplotlib figures: {len(numbers)}")
        for index, number in enumerate(numbers, 1):
            fig = plt.figure(number)
            fig.canvas.draw()
            path = args.out / f"{args.package}-{index:02d}.png"
            fig.savefig(path, dpi=110)
            print(f"[PNG] {path}")
        if not numbers:
            print("[NOTE] No active figures. This does not prove the package lacks plot support; "
                  "the sample may not exercise plots or may use another output path.")
        if args.show and numbers:
            plt.show(block=True)
        return 0
    except Exception as exc:
        print(f"[FAIL] {type(exc).__name__}: {exc}", file=sys.stderr)
        return 1
    finally:
        gretl.set_plot_auto_show(old_show)
        plt.close("all")


if __name__ == "__main__":
    raise SystemExit(main())
