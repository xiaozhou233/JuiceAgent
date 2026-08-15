@echo off
setlocal

set "OUT=out"
set "JAR=JuiceAgent-JavaInjector-1.0-SNAPSHOT.jar"
set "TMP=tmp_jar"

if not exist "%JAR%" (
    echo Error: %JAR% not found. Please restore the original jar.
    exit /b 1
)

if exist "%OUT%" rmdir /s /q "%OUT%"
if exist "%TMP%" rmdir /s /q "%TMP%"

mkdir "%OUT%"
mkdir "%TMP%"

javac -encoding UTF-8 --release 8 -cp "%JAR%" -d "%OUT%" JavaInjector.java
if errorlevel 1 goto :error

rem Extract existing jar (contains Injector/WindowInfo classes) into temp dir
rem -J-XX:-UsePerfData prevents JVM from creating locked hsperfdata files in CWD
pushd "%TMP%"
jar -J-XX:-UsePerfData xf "..\%JAR%"
popd
if errorlevel 1 goto :error

rem Remove any hsperfdata folder that may have leaked into the jar
for /d %%D in ("%TMP%\hsperfdata_*") do rmdir /s /q "%%D"

rem Merge compiled JavaInjector.class into temp dir
copy /y "%OUT%\JavaInjector.class" "%TMP%\JavaInjector.class" >nul
if errorlevel 1 goto :error

rem Repackage with Main-Class
jar -J-XX:-UsePerfData --create --file "%JAR%" --main-class JavaInjector -C "%TMP%" .
if errorlevel 1 goto :error

rmdir /s /q "%OUT%"
rmdir /s /q "%TMP%"

echo Build succeeded: %JAR%
goto :end

:error
echo Build failed.
exit /b 1

:end
endlocal
