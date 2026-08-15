@echo off
setlocal

java -jar target.jar
if errorlevel 1 goto :error

goto :end

:error
echo Run failed.
exit /b 1

:end
endlocal
