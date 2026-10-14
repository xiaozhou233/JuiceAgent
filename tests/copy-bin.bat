@ECHO OFF
SETLOCAL

REM Always operate relative to this script so it works from any working directory.
PUSHD "%~dp0"

IF NOT EXIST "resources" MKDIR "resources"

REM Copy fresh build artifacts into the test resources directory.
COPY /Y "..\build\bin\*.dll" "resources" >NUL
IF ERRORLEVEL 1 (
    ECHO [ERROR] Failed to copy DLLs from ..\build\bin to resources.
    ECHO         Make sure the project is built and that no test JVM is still
    ECHO         holding resources\libagent.dll open.
    POPD
    EXIT /B 1
)

COPY /Y "..\build\bin\*.exe" "resources" >NUL
IF ERRORLEVEL 1 (
    ECHO [ERROR] Failed to copy EXEs from ..\build\bin to resources.
    ECHO         Make sure the project is built before running this script.
    POPD
    EXIT /B 1
)

ECHO [SUCCESS] Copied build binaries to tests\resources.

POPD
EXIT /B 0
