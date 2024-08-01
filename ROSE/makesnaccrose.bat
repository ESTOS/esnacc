@echo off
cls
rem This batch creates the files for the ROSE asn1 envelop used by the cpp and the typescript backends
SET PATH=%PATH%;..\output\bin\

SET EXECUTABLE=

echo Searching for esnacc executable...
where esnaccd.exe 2>nul
IF %ERRORLEVEL% == 0 SET EXECUTABLE=esnaccd.exe>NUL
IF NOT "%EXECUTABLE%" == "" GOTO start
where esnacc.exe 2>nul
IF %ERRORLEVEL% == 0 SET EXECUTABLE=esnacc.exe>NUL
IF NOT "%EXECUTABLE%" == "" GOTO start

SET PATH=%PATH%;..\..\..\buildtools\

where esnacc6.exe 2>nul
IF %ERRORLEVEL% == 0 SET EXECUTABLE=esnacc6.exe>NUL
IF NOT "%EXECUTABLE%" == "" GOTO start

echo Could not find esnaccd.exe or esnacc.exe, please build the compiler first
goto end

:start
echo %EXECUTABLE% -ValidationLevel 0 -C -x -p -e -d -j SNACCROSE.asn1
%EXECUTABLE% -ValidationLevel 0 -C -x -p -e -d -j SNACCROSE.asn1
if NOT %ERRORLEVEL% == 0 pause
move SNACCROSE.cpp ..\cpp-lib\src\SNACCROSE.cpp >NUL
move SNACCROSE.h ..\cpp-lib\include\SNACCROSE.h >NUL

echo %EXECUTABLE% -ValidationLevel 0 -JT -j SNACCROSE.asn1
%EXECUTABLE% -ValidationLevel 0 -JT -j SNACCROSE.asn1
if NOT %ERRORLEVEL% == 0 pause
move SNACCROSE.ts ..\compiler\back-ends\ts-gen\gluecode\SNACCROSE.ts >NUL
move SNACCROSE_Converter.ts ..\compiler\back-ends\ts-gen\gluecode\SNACCROSE_Converter.ts >NUL
del *.ts

:end
pause