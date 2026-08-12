#!/usr/bin/env bash
#
# JuiceAgent release packaging script.
# The binaries and example JARs are committed to the repository, so this
# script only packages them. Runs inside the juiceagent-builder container.
# Produces two artifacts:
#   1. JuiceAgent-windows-x64.zip           - pure compiled binaries
#   2. JuiceAgent-windows-x64-examples.zip  - binaries + runnable example
#
set -e

cd /workspace

EXAMPLE_DIR=examples/load-via-injector-exe

echo "========================================"
echo " [1/2] Packaging pure binaries"
echo "========================================"
rm -f /workspace/JuiceAgent-windows-x64.zip
cd "$EXAMPLE_DIR"
zip -9 -j /workspace/JuiceAgent-windows-x64.zip injector.exe libagent.dll libloader.dll libinject.dll
cd /workspace

echo "========================================"
echo " [2/2] Packaging binaries + example"
echo "========================================"
rm -rf /workspace/release-examples
mkdir -p /workspace/release-examples
cp -r "$EXAMPLE_DIR/." /workspace/release-examples/

rm -f /workspace/JuiceAgent-windows-x64-examples.zip
cd /workspace/release-examples
zip -9 -r /workspace/JuiceAgent-windows-x64-examples.zip .
cd /workspace

echo "========================================"
echo " Done. Artifacts:"
ls -lh /workspace/JuiceAgent-windows-x64.zip /workspace/JuiceAgent-windows-x64-examples.zip
echo "========================================"
