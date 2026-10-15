#!/usr/bin/env bash
# Runs clang-tidy on all of our sources in src/ (E-08).
# Platform files that this build does not compile (e.g. *_windows.cpp on
# Linux) are skipped, since they have no entry in compile_commands.json.
# Usage: tools/run-clang-tidy.sh <build-dir-with-compile_commands.json>
set -euo pipefail

build_dir="${1:?build directory with compile_commands.json required}"
repo_root="$(cd "$(dirname "$0")/.." && pwd)"

cd "$repo_root"
sources=()
while IFS= read -r file; do
    if grep -qF "\"$repo_root/$file\"" "$build_dir/compile_commands.json"; then
        sources+=("$file")
    fi
done < <(find src -name '*.cpp' | sort)
printf '%s\n' "${sources[@]}" | xargs -P "$(nproc)" -n 1 clang-tidy --quiet -p "$build_dir"
