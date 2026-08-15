@echo off
setlocal

set "SRC=target.java"
set "OUT=out"
set "JAR=..\target.jar"

if exist "%OUT%" rmdir /s /q "%OUT%"
if exist "%JAR%" del /q "%JAR%"

mkdir "%OUT%"
javac -encoding UTF-8 --release 8 -d "%OUT%" "%SRC%"
if errorlevel 1 goto :error

jar --create --file "%JAR%" --main-class target -C "%OUT%" .
if errorlevel 1 goto :error

rmdir /s /q "%OUT%"

echo Build succeeded: target.jar
goto :end

:error
echo Build failed.
exit /b 1

:end
endlocal
