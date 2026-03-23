@echo off
cls

SET COMPILER=

echo Searching for esnacc executable in output directory
SET PATH=%PATH%;%CD%\..\output\bin\
where esnaccd.exe 2>nul
IF %ERRORLEVEL% == 0 SET COMPILER=esnaccd.exe
IF NOT "%COMPILER%" == "" GOTO start
where esnacc.exe 2>nul
IF %ERRORLEVEL% == 0 SET COMPILER=esnacc.exe
IF NOT "%COMPILER%" == "" GOTO start

echo Searching for esnacc executable in global buildtools
SET PATH=%PATH%;%CD%\..\..\..\buildtools\
where esnacc7.exe 2>nul
IF %ERRORLEVEL% == 0 SET COMPILER=esnacc7.exe
IF NOT "%COMPILER%" == "" GOTO start

echo Could not find esnaccd.exe or esnacc.exe, please build the compiler first
goto end

:start
echo %COMPILER% -ValidationLevel 0 -C -x -p -e -d -j SNACCROSE.asn1
%COMPILER% -ValidationLevel 0 -C -x -p -e -d -j SNACCROSE.asn1
if NOT %ERRORLEVEL% == 0 pause
move SNACCROSE.cpp ..\cpp-lib\src\SNACCROSE.cpp >NUL
move SNACCROSE.h ..\cpp-lib\include\SNACCROSE.h >NUL

echo %COMPILER% -ValidationLevel 0 -JTE -j SNACCROSE.asn1
%COMPILER% -ValidationLevel 0 -JTE -j SNACCROSE.asn1
if NOT %ERRORLEVEL% == 0 pause
move SNACCROSE.ts ..\compiler\back-ends\ts-gen\gluecode\SNACCROSE.ts >NUL
move SNACCROSE_Converter.ts ..\compiler\back-ends\ts-gen\gluecode\SNACCROSE_Converter.ts >NUL
del *.ts

echo finished...
:end
timeout /t 10
