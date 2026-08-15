@echo off
setlocal

set "API_JAR=playload\JuiceAgent-API-1.4.1+build.1.jar"
set "PAYLOAD_JAR=playload.jar"
set "TARGET_JAR=target.jar"

set "OUT=out"
set "OUT_NT=%OUT%\new_target"
set "OUT_PL=%OUT%\playload"
set "OUT_TG=%OUT%\target"

set "NEW_TARGET_CLASS=new_target.class"

if exist "%OUT%" rmdir /s /q "%OUT%"
if exist "%PAYLOAD_JAR%" del /q "%PAYLOAD_JAR%"
if exist "%TARGET_JAR%" del /q "%TARGET_JAR%"
if exist "%NEW_TARGET_CLASS%" del /q "%NEW_TARGET_CLASS%"

mkdir "%OUT_NT%"
mkdir "%OUT_PL%"
mkdir "%OUT_TG%"

echo === Compiling new_target ===
javac -encoding UTF-8 --release 8 -d "%OUT_NT%" new_target\target.java
if errorlevel 1 goto :error
ren "%OUT_NT%\target.class" new_target.class
if errorlevel 1 goto :error
copy /y "%OUT_NT%\new_target.class" "%NEW_TARGET_CLASS%"
if errorlevel 1 goto :error

echo === Compiling playload ===
javac -encoding UTF-8 --release 8 -cp "%API_JAR%" -d "%OUT_PL%" playload\playload.java
if errorlevel 1 goto :error

echo === Compiling target ===
javac -encoding UTF-8 --release 8 -d "%OUT_TG%" target\target.java
if errorlevel 1 goto :error

echo === Packaging payload jar ===
jar --create --file "%PAYLOAD_JAR%" -C "%OUT_PL%" playload.class
if errorlevel 1 goto :error

echo === Packaging target jar ===
jar --create --file "%TARGET_JAR%" --main-class target -C "%OUT_TG%" .
if errorlevel 1 goto :error

rmdir /s /q "%OUT%"

echo Build succeeded: %PAYLOAD_JAR%, %TARGET_JAR%, %NEW_TARGET_CLASS%
goto :end

:error
echo Build failed.
exit /b 1

:end
endlocal
