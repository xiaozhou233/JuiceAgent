
@echo off
setlocal

set "ROOT_DIR=%~dp0"

cmake -P "%ROOT_DIR%package.cmake"

if errorlevel 1 (
    echo [ERROR] Packaging failed.
    exit /b 1
)

echo [SUCCESS] Packaging completed.

pause