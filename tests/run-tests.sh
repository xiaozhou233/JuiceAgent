#!/usr/bin/env bash
#
# Build and run the JuiceAgent integration tests on Linux.
#
# Mirror of tests/run-tests.ps1: compiles the injection + test modules,
# refreshes tests/resources from build/bin, then runs every case listed in
# tests/cases.txt and asserts on the RESULT: PASS/FAIL verdict each prints
# (the test programs always exit 0, even on failure - see AGENTS.md rule 5).
#
# Requires the native binaries in build/bin; run `cmake --build build` first.
#
# Usage:
#   bash tests/run-tests.sh
#   JAVA_HOME=/path/to/jdk bash tests/run-tests.sh
set -euo pipefail

tests_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$tests_dir/.." && pwd)"

# --- Locate JDK tools ------------------------------------------------------
if [[ -n "${JAVA_HOME:-}" ]]; then
    java="$JAVA_HOME/bin/java"
    javac="$JAVA_HOME/bin/javac"
    jar="$JAVA_HOME/bin/jar"
else
    java="$(command -v java || true)"
    javac="$(command -v javac || true)"
    jar="$(command -v jar || true)"
fi

for tool in "$java" "$javac" "$jar"; do
    if [[ -z "$tool" || ! -x "$tool" ]]; then
        echo "JDK tools not found. Install a JDK (17+) and put it on PATH, or set JAVA_HOME." >&2
        exit 1
    fi
done

# --- JDK version -> native-access flag -------------------------------------
# --enable-native-access exists since JDK 22 and is needed to silence the JNI
# warnings added in JDK 24.
version_text="$("$java" -version 2>&1)"
if [[ "$version_text" =~ version\ \"([0-9]+) ]]; then
    java_major="${BASH_REMATCH[1]}"
else
    echo "Could not parse the Java version from: $version_text" >&2
    exit 1
fi

if (( java_major < 17 )); then
    echo "JDK $java_major is too old; the tests need JDK 17+." >&2
    exit 1
fi

native_access_args=()
if (( java_major >= 22 )); then
    native_access_args=("--enable-native-access=ALL-UNNAMED")
fi

# --- Locate the JuiceAgent API jar -----------------------------------------
api_jar="$(find "$tests_dir/lib" -maxdepth 1 -name 'JuiceAgent-API-*.jar' -print -quit)"
if [[ -z "$api_jar" ]]; then
    echo "JuiceAgent API jar not found under tests/lib." >&2
    exit 1
fi

# --- Require native binaries ----------------------------------------------
bin_dir="$repo_root/build/bin"
if [[ ! -f "$bin_dir/libagent.so" ]]; then
    echo "Missing libagent.so in build/bin. Build the native binaries first: cmake --build build" >&2
    exit 1
fi

echo "=== JuiceAgent integration tests ==="
echo "JDK:      $java (major $java_major)"
echo "API jar:  $api_jar"
echo "Binaries: $bin_dir"
echo

cd "$tests_dir"

# --- 1. Compile the injection module (used by redefine/retransform) --------
injection_out="out/production/injection"
mkdir -p "$injection_out"
"$javac" -d "$injection_out" injection/src/*.java

# --- 2. Rebuild injection.jar (used by the AddTo* tests) -------------------
"$jar" cf injection.jar -C "$injection_out" .

# --- 3. Refresh tests/resources from build/bin -----------------------------
bash copy-bin.sh

# --- 4. Compile the tests (Loop.java is a manual demo, not a test) ---------
tests_out="out/production/tests"
mkdir -p "$tests_out"
test_sources=()
for src in src/*.java; do
    [[ "$(basename "$src")" == "Loop.java" ]] && continue
    test_sources+=("$src")
done
"$javac" -cp "$api_jar" -d "$tests_out" "${test_sources[@]}"

# --- 5. Run every case and assert on the verdict ---------------------------
classpath="$tests_out:$api_jar"
failed=()
total=0
while IFS= read -r case_name || [[ -n "$case_name" ]]; do
    case_name="${case_name%%#*}"              # strip comments
    case_name="$(printf '%s' "$case_name" | xargs)"  # trim
    [[ -z "$case_name" ]] && continue
    total=$((total + 1))

    output="$("$java" "${native_access_args[@]}" -cp "$classpath" "$case_name" 2>&1 || true)"
    if grep -q "RESULT: PASS $case_name" <<<"$output" && ! grep -q "RESULT: FAIL" <<<"$output"; then
        printf '  [PASS] %s\n' "$case_name"
    else
        printf '  [FAIL] %s (expected "RESULT: PASS %s")\n' "$case_name" "$case_name"
        head -n 25 <<<"$output" | sed 's/^/         | /'
        failed+=("$case_name")
    fi
done < cases.txt

echo
if (( ${#failed[@]} > 0 )); then
    echo "${#failed[@]}/$total tests failed: ${failed[*]}" >&2
    exit 1
fi

echo "All $total tests passed."
