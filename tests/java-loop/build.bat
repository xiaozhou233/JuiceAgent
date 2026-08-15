@echo off
setlocal
cd /d "%~dp0"

if not exist build mkdir build

javac -d build loop.java
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [OK] Build succeeded: build\loop.class
endlocal
