@echo off
echo === Building injector jar ===
pushd injector
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
echo Build finished: JuiceAgent-JavaInjector-1.0-SNAPSHOT.jar and target.jar are ready.
goto :end

:error
echo Build failed.
exit /b 1

:end