#!/usr/bin/env bash
#
# JuiceAgent release build script.
# Runs inside the juiceagent-builder Docker container.
# The example JARs are committed to the repository, so only the native
# binaries are compiled here (MinGW cross-compile).
# Produces two artifacts:
#   1. JuiceAgent-windows-x64.zip           - pure compiled binaries
#   2. JuiceAgent-windows-x64-examples.zip  - binaries + runnable example
#
set -e

cd /workspace

EXAMPLE_DIR=examples/load-via-injector-exe

echo "========================================"
echo " [1/3] Building native binaries (MinGW)"
echo "========================================"
rm -rf build-mingw
cmake --preset mingw-release
cmake --build build-mingw

echo "========================================"
echo " [2/3] Packaging pure binaries"
echo "========================================"
rm -f /workspace/JuiceAgent-windows-x64.zip
cd build-mingw/bin
zip -9 -j /workspace/JuiceAgent-windows-x64.zip injector.exe libagent.dll libloader.dll libinject.dll
cd /workspace

echo "========================================"
echo " [3/3] Packaging binaries + example"
echo "========================================"
rm -rf /workspace/release-examples
mkdir -p /workspace/release-examples

cp -r "$EXAMPLE_DIR/." /workspace/release-examples/

cp build-mingw/bin/injector.exe  /workspace/release-examples/
cp build-mingw/bin/libagent.dll  /workspace/release-examples/
cp build-mingw/bin/libloader.dll /workspace/release-examples/
cp build-mingw/bin/libinject.dll /workspace/release-examples/

rm -f /workspace/JuiceAgent-windows-x64-examples.zip
cd /workspace/release-examples
zip -9 -r /workspace/JuiceAgent-windows-x64-examples.zip .
cd /workspace

echo "========================================"
echo " Done. Artifacts:"
ls -lh /workspace/JuiceAgent-windows-x64.zip /workspace/JuiceAgent-windows-x64-examples.zip
echo "========================================"
