@echo off
cd /d "%~dp0"

set "INJECTOR_JAR=injector\JuiceAgent-JavaInjector-1.0-SNAPSHOT.jar"

echo === Java API Injector ===
echo.
echo Make sure the target JVM is running (run.bat).
echo.
echo Usage: This script runs the JavaInjector which will prompt for:
echo   - PID: target JVM process ID
echo   - DLL: path to libloader.dll
echo   - config_path: directory containing config.toml (leave empty to use current dir)
echo.

java -cp "%INJECTOR_JAR%" JavaInjector
pause