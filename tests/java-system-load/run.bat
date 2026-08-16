@echo off
setlocal
cd /d "%~dp0"

java -cp build Loader
if errorlevel 1 (
    echo [ERROR] Run failed.
    exit /b 1
)

endlocal