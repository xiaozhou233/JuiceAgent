@echo off
echo === Building injection jar ===
pushd injection
call build.bat
popd
if errorlevel 1 goto :error

echo.
echo === Building target jar ===
pushd target
call build.bat
popd
if errorlevel 1 goto :error

echo.
echo Build finished: demo.jar and target.jar are in this directory.
goto :end

:error
echo Build failed.
exit /b 1

:end
