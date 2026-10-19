#!/usr/bin/env bash
#
# Linux counterpart of script/package.bat.
#
# Packs the Linux build in build/bin into
#   dist/JuiceAgent_v<major>.<minor>.<patch>_x64_linux.tar.gz
#
# Usage: ./script/package.sh
#
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

VERSION_FILE="${PROJECT_DIR}/Version.txt"
BIN_DIR="${PROJECT_DIR}/build/bin"
OUTPUT_DIR="${PROJECT_DIR}/dist"

# Read one "Key = value" entry from Version.txt.
read_version_key() {
    local key="$1"
    sed -n -E "s/^[[:space:]]*${key}[[:space:]]*=[[:space:]]*([0-9]+)[[:space:]]*$/\1/p" \
        "${VERSION_FILE}" | head -n 1
}

if [[ ! -f "${VERSION_FILE}" ]]; then
    echo "[ERROR] Version.txt not found: ${VERSION_FILE}" >&2
    exit 1
fi

VERSION_MAJOR="$(read_version_key Major)"
VERSION_MINOR="$(read_version_key Minor)"
VERSION_PATCH="$(read_version_key Patch)"

if [[ -z "${VERSION_MAJOR}" || -z "${VERSION_MINOR}" || -z "${VERSION_PATCH}" ]]; then
    echo "[ERROR] Invalid Version.txt: Major, Minor and Patch are required" >&2
    exit 1
fi

# Linux build artifacts (there is no libinject on this platform).
PACKAGE_FILES=(
    injector
    libloader.so
    libagent.so
)

for file_name in "${PACKAGE_FILES[@]}"; do
    if [[ ! -f "${BIN_DIR}/${file_name}" ]]; then
        echo "[ERROR] Required file not found: ${BIN_DIR}/${file_name}" >&2
        echo "        Build first: cmake --preset release && cmake --build build" >&2
        exit 1
    fi
done

mkdir -p "${OUTPUT_DIR}"

OUTPUT_FILE="${OUTPUT_DIR}/JuiceAgent_v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}_x64_linux.tar.gz"

rm -f "${OUTPUT_FILE}"

tar -czf "${OUTPUT_FILE}" -C "${BIN_DIR}" "${PACKAGE_FILES[@]}"

echo "[SUCCESS] Package created: ${OUTPUT_FILE}"
