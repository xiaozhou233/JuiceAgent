@echo off
setlocal
cd /d "%~dp0"

set "SRC=..\..\build\bin"

echo [INFO] Copying build artifacts from "%SRC%" ...
xcopy /Y "%SRC%\libagent.dll" .\ >nul
xcopy /Y "%SRC%\libinject.dll" .\ >nul
xcopy /Y "%SRC%\libloader.dll" .\ >nul
echo [OK]  Artifacts copied.

if not exist build mkdir build

javac -d build Loader.java
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [OK] Build succeeded: build\Loader.class

endlocal