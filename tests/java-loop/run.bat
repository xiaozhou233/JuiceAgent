@echo off
setlocal
cd /d "%~dp0"

if not exist build\loop.class (
    echo [ERROR] Build output not found. Run build.bat first.
    exit /b 1
)

java -cp build loop
if errorlevel 1 (
    echo [ERROR] Run failed.
    exit /b 1
)

endlocal
