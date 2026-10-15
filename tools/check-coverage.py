#!/usr/bin/env python3
"""Coverage gate (E-02).

Runs gcovr on a coverage build and fails unless every file under src/ is either
covered to 100 % (lines and branches) or listed in coverage-exclusions.txt.
Writes HTML and Cobertura reports to <build-dir>/coverage/.

Usage: tools/check-coverage.py <coverage-build-dir>
"""

import json
import pathlib
import subprocess
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE_SUFFIXES = {".h", ".hpp", ".cpp"}


def read_exclusions():
    exclusions = set()
    for line in (REPO_ROOT / "coverage-exclusions.txt").read_text(encoding="utf-8").splitlines():
        entry = line.split("#", 1)[0].strip()
        if entry:
            exclusions.add(entry)
    return exclusions


def source_files():
    return {
        path.relative_to(REPO_ROOT).as_posix()
        for path in (REPO_ROOT / "src").rglob("*")
        if path.suffix in SOURCE_SUFFIXES
    }


def has_measured_source(header, measured):
    """A header without executable lines is covered through its .cpp file."""
    stem, suffix = header.rsplit(".", 1)
    return suffix in ("h", "hpp") and f"{stem}.cpp" in measured


def run_gcovr(build_dir, exclusions):
    report_dir = build_dir / "coverage"
    report_dir.mkdir(exist_ok=True)
    summary_file = report_dir / "summary.json"
    command = [
        "gcovr",
        "--root", str(REPO_ROOT),
        "--object-directory", str(build_dir),
        "--filter", str(REPO_ROOT / "src") + "/",
        "--exclude-throw-branches",
        # GCC attributes exception cleanup of returned locals to the closing brace.
        "--exclude-noncode-lines",
        "--exclude-unreachable-branches",
        # A template line is covered if any instantiation covers it (e.g. SnapshotExchange).
        "--merge-lines",
        # Lines like a lone "}" only carry compiler-generated exception cleanup.
        "--exclude-noncode-lines",
        "--print-summary",
        "--txt", "-",
        "--html-details", str(report_dir / "index.html"),
        "--cobertura", str(report_dir / "coverage.xml"),
        "--json-summary", str(summary_file),
        "--fail-under-line", "100",
        "--fail-under-branch", "100",
    ]
    for entry in sorted(exclusions):
        command += ["--exclude", str(REPO_ROOT / entry)]
    result = subprocess.run(command, check=False)
    return result.returncode, json.loads(summary_file.read_text(encoding="utf-8"))


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    build_dir = pathlib.Path(sys.argv[1]).resolve()

    exclusions = read_exclusions()
    sources = source_files()
    stale = sorted(exclusions - sources)
    gcovr_status, summary = run_gcovr(build_dir, exclusions)

    measured = {entry["filename"] for entry in summary["files"]}
    unmeasured = sorted(path for path in sources - exclusions - measured if not has_measured_source(path, measured))

    ok = gcovr_status == 0
    if stale:
        ok = False
        print("coverage-exclusions.txt lists files that do not exist:", *stale, sep="\n  ")
    if unmeasured:
        ok = False
        print("Files under src/ without test coverage and not listed in coverage-exclusions.txt:",
              *unmeasured, sep="\n  ")
    if gcovr_status != 0:
        print("Coverage is below 100 % (lines or branches).")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
