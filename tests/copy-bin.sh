#!/usr/bin/env bash
#
# Refresh tests/resources with the freshly built native binaries (Linux).
#
# Mirror of tests/copy-bin.bat: always operates relative to this script and
# fails loudly when there is nothing to copy.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$here"

mkdir -p resources

shopt -s nullglob
libs=(../build/bin/*.so)
if (( ${#libs[@]} == 0 )); then
    echo "[ERROR] No shared libraries found in ../build/bin." >&2
    echo "        Build the project first: cmake --build build" >&2
    exit 1
fi

cp -f "${libs[@]}" resources/

echo "[SUCCESS] Copied build binaries to tests/resources."
