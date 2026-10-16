<#
.SYNOPSIS
    Build and run the JuiceAgent integration tests.

.DESCRIPTION
    Compiles the test + injection modules, refreshes tests/resources from
    build/bin, then runs every test and asserts on its output.

    The test programs always exit 0, even when a step fails (e.g. a bad config
    or a missing class is printed but the process still returns 0), so exit
    codes alone are not enough - we match an expected marker per test.

    Requires the native binaries in build/bin; run `cmake --build build` first.

.PARAMETER JdkHome
    JDK home directory. Defaults to the `java`/`javac`/`jar` on PATH.

.EXAMPLE
    pwsh -File tests/run-tests.ps1
    pwsh -File tests/run-tests.ps1 -JdkHome "C:\Program Files\Zulu\zulu-25"
#>
[CmdletBinding()]
param(
    [string]$JdkHome = ""
)

# Native commands (java/javac/jar) write to stderr for warnings; with
# ErrorActionPreference=Stop that would be treated as a terminating error on
# Windows PowerShell 5.1. Failures are detected explicitly via $LASTEXITCODE
# and the per-test output assertions instead.
$ErrorActionPreference = "Continue"

$TestsDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $TestsDir

function Resolve-FromPath([string]$name) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

# --- Locate JDK tools ------------------------------------------------------
if ($JdkHome) {
    $java  = Join-Path $JdkHome "bin\java.exe"
    $javac = Join-Path $JdkHome "bin\javac.exe"
    $jar   = Join-Path $JdkHome "bin\jar.exe"
} else {
    $java  = Resolve-FromPath "java"
    $javac = Resolve-FromPath "javac"
    $jar   = Resolve-FromPath "jar"
}

foreach ($tool in @($java, $javac, $jar)) {
    if (-not $tool -or -not (Test-Path $tool)) {
        throw "JDK tools not found. Install a JDK (17+) and put it on PATH, or pass -JdkHome <dir>."
    }
}

# --- JDK version -> native-access flag -------------------------------------
# --enable-native-access exists since JDK 22 and is needed to silence the JNI
# warnings added in JDK 24.
$versionText = (& $java -version 2>&1 | Out-String)
if ($versionText -notmatch 'version "(\d+)') {
    throw "Could not parse the Java version from: $versionText"
}
$javaMajor = [int]$Matches[1]
if ($javaMajor -lt 17) {
    throw "JDK $javaMajor is too old; the tests need JDK 17+."
}

$nativeAccessArgs = @()
if ($javaMajor -ge 22) {
    $nativeAccessArgs = @("--enable-native-access=ALL-UNNAMED")
}

# --- Locate the JuiceAgent API jar -----------------------------------------
$apiJar = Get-ChildItem (Join-Path $TestsDir "lib") -Filter "JuiceAgent-API-*.jar" -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $apiJar) {
    throw "JuiceAgent API jar not found under tests/lib."
}
$apiJar = $apiJar.FullName

# --- Require native binaries ----------------------------------------------
$binDir = Join-Path $RepoRoot "build\bin"
foreach ($dll in @("libagent.dll", "libloader.dll", "libinject.dll", "injector.exe")) {
    if (-not (Test-Path (Join-Path $binDir $dll))) {
        throw "Missing $dll in build\bin. Build the native binaries first: cmake --build build"
    }
}

Write-Host "=== JuiceAgent integration tests ===" -ForegroundColor Cyan
Write-Host ("JDK:      {0} (major {1})" -f $java, $javaMajor)
Write-Host ("API jar:  {0}" -f $apiJar)
Write-Host ("Binaries: {0}" -f $binDir)
Write-Host ""

Push-Location $TestsDir
try {
    # --- 1. Compile the injection module (used by redefine/retransform) ----
    $injectionOut = "out\production\injection"
    New-Item -ItemType Directory -Force -Path $injectionOut | Out-Null
    $injectionSources = (Get-ChildItem "injection\src" -Filter *.java).FullName
    & $javac -d $injectionOut $injectionSources
    if ($LASTEXITCODE -ne 0) { throw "Compiling the injection module failed." }

    # --- 2. Rebuild injection.jar (used by the AddTo* tests) --------------
    & $jar cf "injection.jar" -C $injectionOut .
    if ($LASTEXITCODE -ne 0) { throw "Rebuilding injection.jar failed." }

    # --- 3. Refresh tests/resources from build/bin ------------------------
    & cmd /c "copy-bin.bat"
    if ($LASTEXITCODE -ne 0) { throw "copy-bin.bat failed." }

    # --- 4. Compile the tests ---------------------------------------------
    $testsOut = "out\production\tests"
    New-Item -ItemType Directory -Force -Path $testsOut | Out-Null
    $testSources = (Get-ChildItem "src" -Filter *.java).FullName
    & $javac -cp $apiJar -d $testsOut $testSources
    if ($LASTEXITCODE -ne 0) { throw "Compiling the tests failed." }

    # --- 5. Run every test and assert on a marker -------------------------
    $classpath = "$testsOut;$apiJar"
    $cases = @(
        @{ Name = "GetClassByName";                  Expect = "class java\.lang\.String" },
        @{ Name = "GetLoadedClasses";                Expect = "class " },
        @{ Name = "GetClassBytes";                   Expect = "Bytes: 399" },
        @{ Name = "GetClassBytesByName";             Expect = "Bytes: 399" },
        @{ Name = "DefineClass";                     Expect = "Injection Invoked!" },
        @{ Name = "RedefineClass";                   Expect = "Modified!" },
        @{ Name = "RedefineClassByName";             Expect = "Modified!" },
        @{ Name = "RetransformClass";                Expect = "Modified!" },
        @{ Name = "RetransformClassByName";          Expect = "Modified!" },
        @{ Name = "AddToClassLoader";                Expect = "Injection Invoked!" },
        @{ Name = "AddToSystemClassLoaderSearch";    Expect = "Injection Invoked!" },
        @{ Name = "AddToBootstrapClassLoaderSearch"; Expect = "Injection Invoked!" }
    )

    $failed = @()
    foreach ($case in $cases) {
        $output = (& $java @nativeAccessArgs -cp $classpath $case.Name 2>&1 | Out-String)
        if ($output -match $case.Expect) {
            Write-Host ("  [PASS] {0}" -f $case.Name) -ForegroundColor Green
        } else {
            Write-Host ("  [FAIL] {0} (expected /{1}/)" -f $case.Name, $case.Expect) -ForegroundColor Red
            # Show what the test actually printed so failures are diagnosable
            # (the test programs exit 0 even when a step fails).
            $lines = @($output -split "`r?`n" | Where-Object { $_.Trim() })
            foreach ($line in ($lines | Select-Object -First 25)) {
                Write-Host ("         | " + $line.TrimEnd()) -ForegroundColor DarkGray
            }
            if ($lines.Count -gt 25) {
                Write-Host ("         | ... ({0} more lines)" -f ($lines.Count - 25)) -ForegroundColor DarkGray
            }
            $failed += $case.Name
        }
    }

    Write-Host ""
    if ($failed.Count -gt 0) {
        Write-Host ("{0}/{1} tests failed: {2}" -f $failed.Count, $cases.Count, ($failed -join ", ")) -ForegroundColor Red
        exit 1
    }

    Write-Host ("All {0} tests passed." -f $cases.Count) -ForegroundColor Green
    exit 0
}
finally {
    Pop-Location
}
