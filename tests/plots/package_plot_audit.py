#!/usr/bin/env python3
"""Inventory gretl function packages and locate code that can generate plots.

This tool performs static inspection only: it NEVER executes downloaded packages.
The reported categories are candidates for human review, not proof of runtime plotting.

Python 3.11+; standard library only.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from html.parser import HTMLParser
import io
import json
from pathlib import Path
import re
import shutil
import sys
import time
from urllib.error import HTTPError, URLError
from urllib.parse import urljoin, urlparse, unquote
from urllib.request import Request, urlopen
import xml.etree.ElementTree as ET
from zipfile import BadZipFile, ZipFile

from audit_config import AUDIT_ROOT

CONTRIBUTED_URL = "https://gretl.sourceforge.net/cgi-bin/gretldata.cgi"
ADDONS_INDEX_URL = "https://gretl.sourceforge.net/addons-data/addons.xml"
GITHUB_ADDONS_URL = "https://api.github.com/repos/gretl-project/gretl/contents/addons"
MAX_DOWNLOAD = 24 * 1024 * 1024
MAX_TEXT_MEMBER = 5 * 1024 * 1024
EXTENSIONS = {".gfn", ".zip"}
TEXT_EXTENSIONS = {".gfn", ".inp", ".spec", ".gp", ".txt"}

# Match gretl/hansl plotting commands when they occur as commands, not substrings.
PLOT_COMMAND = re.compile(
    r"^\s*(?:gnuplot|boxplot|scatters|tsplots|gridplot|gpbuild|"
    r"panplot|hfplot|qqplot|kdplot|rmplot|plot)\b", re.I
)
# These commands may generate a plot, depending on their actual options.
OPTIONAL_PLOT_COMMAND = re.compile(
    r"^\s*(?:corr|corrgm|xcorrgm|freq|fcast|cusum|qlrtest|"
    r"hurst|leverage|pergm|modtest|bootstrap)\b.*(?:--plot|--output)", re.I
)
GNUPLOT_SOURCE = re.compile(
    r"\b(?:set\s+(?:terminal|term|multiplot|style|object)|"
    r"splot\b|filledcurves\b|plot\s+['\"]?-|outfile\b|"
    r"gnuplot\b|gpbuild\b|gridplot\b|--band\b|--bands\b)\b", re.I
)
# Syntax may be assembled dynamically and is therefore less conclusive.
PLOT_MENTION = re.compile(r"\b(?:plot|graph|figure|chart|image|gpfile)\b", re.I)
PLOT_LIKE_NAME = re.compile(r"(?:plot|graph|chart|display|visual|draw|show|irf|fevd|hd)", re.I)
FUNC_TAG = re.compile(r"(?:^|[-_])function$", re.I)


@dataclass
class FileEvidence:
    package: str
    origin: str
    file: str
    function: str
    line: int
    kind: str
    statement: str


class PackageHTMLParser(HTMLParser):
    """Collect linked .gfn and .zip packages from the contributed list."""

    def __init__(self) -> None:
        super().__init__()
        self.links: dict[str, str] = {}

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        if tag.lower() != "a":
            return
        href = dict(attrs).get("href")
        if not href:
            return
        url = urljoin(CONTRIBUTED_URL, href)
        stem = Path(unquote(urlparse(url).path)).name
        if Path(stem).suffix.lower() in EXTENSIONS:
            self.links[stem] = url


def fetch_bytes(url: str, limit: int = MAX_DOWNLOAD, retries: int = 2) -> bytes:
    """Retrieve a bounded payload with retry and a descriptive failure."""
    last_exc: Exception | None = None
    for attempt in range(retries + 1):
        try:
            req = Request(url, headers={"User-Agent": "gretl4py-package-plot-audit/0.1"})
            with urlopen(req, timeout=30) as response:
                data = response.read(limit + 1)
            if len(data) > limit:
                raise ValueError(f"Download exceeds size limit: {url}")
            return data
        except (HTTPError, URLError, TimeoutError, OSError) as exc:
            last_exc = exc
            if attempt < retries:
                time.sleep(attempt + 1)
    raise RuntimeError(f"Could not download {url}: {last_exc}")


def catalog_from_html(html: str) -> dict[str, str]:
    parser = PackageHTMLParser()
    parser.feed(html)
    return parser.links


def catalog_from_addons_xml(xml_bytes: bytes) -> dict[str, str]:
    """Best-effort reading of official addon index; allow unknown XML schema."""
    root = ET.fromstring(xml_bytes)
    result: dict[str, str] = {}
    for node in root.iter():
        candidates = list(node.attrib.values())
        if node.text and len(node.text.strip()) < 100:
            candidates.append(node.text.strip())
        for value in candidates:
            if not value:
                continue
            # Addons index may contain names without file extensions.
            match = re.search(r"(?:^|[/\\])([\w.-]+\.(?:gfn|zip))\b", value, re.I)
            if match:
                name = match.group(1)
                result[name] = urljoin(ADDONS_INDEX_URL, name)
    return result


def github_addons_catalog() -> dict[str, str]:
    """Download official repository's addon source files as a fallback."""
    payload = json.loads(fetch_bytes(GITHUB_ADDONS_URL, limit=5 * 1024 * 1024))
    result: dict[str, str] = {}
    for item in payload:
        if item.get("type") != "file":
            continue
        name = item.get("name", "")
        if Path(name).suffix.lower() in TEXT_EXTENSIONS:
            url = item.get("download_url")
            if url:
                result[name] = url
    return result


def safe_output_filename(name: str) -> str:
    """Prevent names from turning into paths."""
    leaf = Path(name).name
    if not leaf or leaf in {".", ".."} or leaf != name:
        raise ValueError(f"Invalid package filename: {name}")
    return leaf


def download_catalog(catalog: dict[str, str], target: Path, manifest: list[dict], origin: str,
                     limit: int | None = None) -> None:
    target.mkdir(parents=True, exist_ok=True)
    for i, (name, url) in enumerate(sorted(catalog.items()), 1):
        if limit is not None and i > limit:
            break
        name = safe_output_filename(name)
        dest = target / name
        record = {"package": Path(name).stem, "origin": origin, "file": name, "url": url,
                  "status": "cached" if dest.is_file() else "pending"}
        if not dest.is_file():
            try:
                payload = fetch_bytes(url)
                temporary = dest.with_name(dest.name + ".tmp")
                temporary.write_bytes(payload)
                temporary.replace(dest)
                record["status"] = "ok"
            except Exception as exc:
                record["status"] = "error"
                record["reason"] = str(exc)
        manifest.append(record)
        print(f"[{origin} {i}/{min(len(catalog), limit or len(catalog))}] "
              f"{name}: {record['status']}", file=sys.stderr)


def cmd_fetch(args: argparse.Namespace) -> None:
    root = args.root
    root.mkdir(parents=True, exist_ok=True)
    manifest: list[dict] = []
    if args.contributed:
        try:
            cat = catalog_from_html(fetch_bytes(CONTRIBUTED_URL, limit=4 * 1024 * 1024).decode("utf-8", "replace"))
            print(f"Contributed packages linked: {len(cat)}", file=sys.stderr)
            download_catalog(cat, root / "contributed", manifest, "contributed", args.limit)
        except Exception as exc:
            print(f"ERROR: contributed catalog: {exc}", file=sys.stderr)
    if args.addons:
        try:
            cat = catalog_from_addons_xml(fetch_bytes(ADDONS_INDEX_URL, limit=2 * 1024 * 1024))
            print(f"Official addons indexed: {len(cat)}", file=sys.stderr)
            if cat:
                download_catalog(cat, root / "addons", manifest, "addons", args.limit)
            else:
                print("No .gfn/.zip URLs in official index; inspect local addons or Github source.", file=sys.stderr)
        except Exception as exc:
            print(f"WARNING: official addon index unavailable: {exc}", file=sys.stderr)
        if args.github_fallback:
            try:
                cat = github_addons_catalog()
                print(f"GitHub addon source files: {len(cat)}", file=sys.stderr)
                download_catalog(cat, root / "addons_source", manifest, "addons_source", args.limit)
            except Exception as exc:
                print(f"WARNING: GitHub addon source unavailable: {exc}", file=sys.stderr)
    write_csv(root / "downloads.csv", manifest,
              ["package", "origin", "file", "url", "status", "reason"])
    print(f"Download manifest: {root / 'downloads.csv'}")


def tag_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1].lower()


def extract_gfn_sections(data: bytes) -> list[tuple[str, str, str]]:
    """Return (section kind, function name, hansl source) from a .gfn XML file."""
    root = ET.fromstring(data)
    sections: list[tuple[str, str, str]] = []
    for el in root.iter():
        t = tag_name(el.tag)
        if t in {"sample-script", "sample_script", "sample-script-text"}:
            body = "".join(el.itertext())
            if body.strip():
                sections.append(("sample", "<sample>", body))
        elif t.endswith("function") and t not in {"gretl-function-package", "function-package"}:
            name = el.attrib.get("name", "<unknown>")
            code_nodes = [child for child in el if tag_name(child.tag) in {"code", "function-code", "source"}]
            for code in code_nodes:
                body = "".join(code.itertext())
                if body.strip():
                    sections.append(("function", name, body))
    # Some old packages use alternate XML elements. Flag them for manual review.
    if not sections:
        for el in root.iter():
            if tag_name(el.tag) in {"code", "sample-script", "source"}:
                body = "".join(el.itertext())
                if body.strip():
                    sections.append(("fallback", el.attrib.get("name", "<unclassified>"), body))
    return sections


def classify_line(line: str) -> str | None:
    stripped = line.strip()
    if not stripped or stripped.startswith("#"):
        return None
    if PLOT_COMMAND.search(stripped):
        return "direct_plot"
    if OPTIONAL_PLOT_COMMAND.search(stripped):
        return "optional_plot"
    if GNUPLOT_SOURCE.search(stripped):
        return "gnuplot_source"
    if PLOT_MENTION.search(stripped):
        return "possible_plot"
    return None


def source_evidence(package: str, origin: str, filename: str, data: bytes) -> tuple[list[FileEvidence], int, int]:
    """Inspect executable hansl in XML and standalone sample scripts."""
    parts: list[tuple[str, str, str]]
    suffix = Path(filename).suffix.lower()
    if suffix == ".gfn":
        try:
            parts = extract_gfn_sections(data)
        except ET.ParseError:
            return [], 0, 1
    else:
        text = data.decode("utf-8", "replace")
        parts = [("sample", "<standalone>", text)]
    evidence: list[FileEvidence] = []
    for kind, name, source in parts:
        for lineno, line in enumerate(source.splitlines(), 1):
            clue = classify_line(line)
            if clue is None:
                continue
            evidence.append(FileEvidence(package, origin, filename, name, lineno, clue, line.strip()[:260]))
    return evidence, len(parts), 0


def iter_files(root: Path):
    """Yield paths in one flat archive and nested ZIP members without extraction."""
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        if path.name.endswith(".tmp") or path.suffix.lower() not in TEXT_EXTENSIONS | {".zip"}:
            continue
        relative = path.relative_to(root)
        origin = relative.parts[0] if len(relative.parts) > 1 else "local"
        if path.suffix.lower() == ".zip":
            try:
                with ZipFile(path) as z:
                    for item in z.infolist():
                        if item.is_dir() or item.file_size > MAX_TEXT_MEMBER:
                            continue
                        if Path(item.filename).suffix.lower() not in TEXT_EXTENSIONS:
                            continue
                        if ".." in Path(item.filename).parts:
                            continue
                        yield path.stem, origin, f"{relative}!{item.filename}", z.read(item)
            except (BadZipFile, OSError, RuntimeError) as exc:
                print(f"Skipping damaged archive {path}: {exc}", file=sys.stderr)
        elif path.stat().st_size <= MAX_TEXT_MEMBER:
            yield path.stem if path.suffix == ".gfn" else path.parent.name, origin, str(relative), path.read_bytes()


def write_csv(path: Path, rows: list[dict], columns: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=columns, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def cmd_scan(args: argparse.Namespace) -> None:
    roots = [args.root, *args.extra]
    all_evidence: list[dict] = []
    packages: dict[tuple[str, str], dict] = {}
    for root in roots:
        if not root.is_dir():
            print(f"Directory not found: {root}", file=sys.stderr)
            continue
        for package, origin, path, data in iter_files(root):
            key = (origin, package)
            state = packages.setdefault(key, {"package": package, "origin": origin,
                                              "files_scanned": 0, "sections": 0,
                                              "parse_errors": 0, "direct_plot": 0,
                                              "optional_plot": 0, "gnuplot_source": 0,
                                              "possible_plot": 0, "sample_has_plot": False})
            evidence, sections, errors = source_evidence(package, origin, path, data)
            state["files_scanned"] += 1
            state["sections"] += sections
            state["parse_errors"] += errors
            for match in evidence:
                state[match.kind] += 1
                if (match.function.startswith("<sample>") or match.function == "<standalone>") and match.kind != "possible_plot":
                    state["sample_has_plot"] = True
                all_evidence.append(vars(match))
    summary = []
    for state in packages.values():
        if state["direct_plot"] or state["optional_plot"]:
            status = "plot_detected"
        elif state["gnuplot_source"]:
            status = "gnuplot_code_candidate"
        elif state["possible_plot"]:
            status = "review_required"
        else:
            status = "no_plot_detected"
        state["status"] = status
        summary.append(state)
    summary.sort(key=lambda p: (-p["direct_plot"], -p["gnuplot_source"], p["package"]))
    cols = ["package", "origin", "status", "files_scanned", "sections", "parse_errors",
            "direct_plot", "optional_plot", "gnuplot_source", "possible_plot", "sample_has_plot"]
    write_csv(args.root / "plot_packages.csv", summary, cols)
    write_csv(args.root / "plot_evidence.csv", all_evidence,
              ["package", "origin", "file", "function", "line", "kind", "statement"])
    selected = [p for p in summary if p["status"] != "no_plot_detected"]
    (args.root / "plot_candidates.json").write_text(json.dumps(selected, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"Scanned {len(summary)} package groups; {len(selected)} candidates; "
          f"{len(all_evidence)} matching source lines.")
    print(f"Outputs: {args.root / 'plot_packages.csv'}, {args.root / 'plot_evidence.csv'}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--root", type=Path, default=AUDIT_ROOT, help="Working directory")
    sub = parser.add_subparsers(dest="command", required=True)
    fetch = sub.add_parser("fetch", help="Download contributed packages and official addons")
    fetch.add_argument("--contributed", action=argparse.BooleanOptionalAction, default=True)
    fetch.add_argument("--addons", action=argparse.BooleanOptionalAction, default=True)
    fetch.add_argument("--github-fallback", action=argparse.BooleanOptionalAction, default=True)
    fetch.add_argument("--limit", type=int, help="Download first N packages from each source (smoke test)")
    sub.add_parser("scan", help="Statically inspect downloaded packages").add_argument(
        "--extra", type=Path, nargs="*", default=[], help="Other local addon/package directories")
    args = parser.parse_args()
    if args.command == "fetch":
        cmd_fetch(args)
    else:
        cmd_scan(args)


if __name__ == "__main__":
    main()
