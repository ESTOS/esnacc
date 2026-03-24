@echo off

SET NODEVERSION=24
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

set FILES=%CD%\interface\*.asn1

rem Building browser client stubs...
echo Building browser client stubs...
PUSHD ts-microservice\client\src\stub
SET COMMAND=-JTE -j -RTS_CLIENT_BROWSER -node:%NODEVERSION% -comments -versionfile %FILES%
echo %COMPILER% %COMMAND%
%COMPILER% %COMMAND%
echo.
if NOT %ERRORLEVEL% == 0 echo Build has failed, check console...>&2
POPD

rem Building node server stubs...
echo Building node server stubs...
PUSHD ts-microservice\server\src\stub
SET COMMAND=-JTE -j -RTS_SERVER -node:%NODEVERSION% -comments -versionfile %FILES%
echo %COMPILER% %COMMAND%
%COMPILER% %COMMAND%
echo.
if NOT %ERRORLEVEL% == 0 echo Build has failed, check console...>&2
POPD

rem Building openapi files...
echo Building openapi files...
PUSHD ts-microservice\openapi\example
SET COMMAND=-JO -node:%NODEVERSION% -comments -versionfile %FILES%
echo %COMPILER% %COMMAND%
%COMPILER% %COMMAND%
echo.
if NOT %ERRORLEVEL% == 0 echo Build has failed, check console...>&2
POPD
PUSHD ts-microservice\openapi\unpkg-example
SET COMMAND=-JO -node:%NODEVERSION% -comments -versionfile %FILES%
echo %COMPILER% %COMMAND%
%COMPILER% %COMMAND%
echo.
if NOT %ERRORLEVEL% == 0 echo Build has failed, check console...>&2
POPD

echo finished...
:end
timeout /t 10