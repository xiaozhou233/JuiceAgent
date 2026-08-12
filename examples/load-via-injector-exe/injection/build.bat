@echo off
setlocal

set "SRC=Main.java"
set "OUT=out"
set "JAR=..\demo.jar"

if exist "%OUT%" rmdir /s /q "%OUT%"
if exist "%JAR%" del /q "%JAR%"

mkdir "%OUT%"
javac -encoding UTF-8 --release 8 -d "%OUT%" "%SRC%"
if errorlevel 1 goto :error

jar --create --file "%JAR%" -C "%OUT%" .
if errorlevel 1 goto :error

rmdir /s /q "%OUT%"

echo Build succeeded: demo.jar
goto :end

:error
echo Build failed.
exit /b 1

:end
endlocal
