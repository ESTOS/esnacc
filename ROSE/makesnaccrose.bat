@echo off
setlocal EnableExtensions

set "COMPILER="
call "%~dp0..\scripts\ensure_compiler.bat"
if errorlevel 1 goto end
set "COMPILER=%SNACC_COMPILER%"

echo %COMPILER% -ValidationLevel 0 -C -x -p -e -d -j SNACCROSE.asn1
"%COMPILER%" -ValidationLevel 0 -C -x -p -e -d -j SNACCROSE.asn1
if NOT %ERRORLEVEL% == 0 pause
move SNACCROSE.cpp ..\cpp-lib\src\SNACCROSE.cpp >NUL
move SNACCROSE.h ..\cpp-lib\include\SNACCROSE.h >NUL

echo %COMPILER% -ValidationLevel 0 -JTE -j SNACCROSE.asn1
"%COMPILER%" -ValidationLevel 0 -JTE -j SNACCROSE.asn1
if NOT %ERRORLEVEL% == 0 pause
move SNACCROSE.ts ..\compiler\back-ends\ts-gen\gluecode\SNACCROSE.ts >NUL
move SNACCROSE_Converter.ts ..\compiler\back-ends\ts-gen\gluecode\SNACCROSE_Converter.ts >NUL
del *.ts

echo finished...
:end
timeout /t 10
