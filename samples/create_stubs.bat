@echo off

rem This batch compiles the asn1 sample interface all the samples
SET NODEVERSION=22

rem resolve the binary path the compiler should be build into
SET COMPILERPATH=%CD%\..\output\bin
IF NOT EXIST %COMPILERPATH% GOTO error
PUSHD %COMPILERPATH%
SET COMPILERPATH=%CD%
POPD
SET PATH=%PATH%;%COMPILERPATH%

rem resolve the compiler inside the binary path
IF EXIST %COMPILERPATH%\esnaccd.exe SET COMPILER=esnaccd.exe
IF EXIST %COMPILERPATH%\esnacc.exe SET COMPILER=esnacc.exe
IF "%COMPILER%" == "" GOTO error
echo Using %COMPILER% as compiler...
echo.

set FILES=%CD%\interface\*.asn1

rem Building browser client stubs...
echo Building browser client stubs...
PUSHD ts-microservice\client\src\stub
SET COMMAND=-JT -j -RTS_CLIENT_BROWSER %FILES%
echo %COMPILER% %COMMAND%
%COMPILER% %COMMAND%
echo.
if NOT %ERRORLEVEL% == 0 echo Build has failed, check console...>&2
POPD

rem Building node server stubs...
echo Building node server stubs...
PUSHD ts-microservice\server\src\stub
SET COMMAND=-JT -j -RTS_SERVER -node:%NODEVERSION% %FILES%
echo %COMPILER% %COMMAND%
%COMPILER% %COMMAND%
echo.
if NOT %ERRORLEVEL% == 0 echo Build has failed, check console...>&2
POPD

rem Building openapi files...
echo Building openapi files...
PUSHD ts-microservice\openapi\example
SET COMMAND=-JO -node:%NODEVERSION% %FILES%
echo %COMPILER% %COMMAND%
%COMPILER% %COMMAND%
echo.
if NOT %ERRORLEVEL% == 0 echo Build has failed, check console...>&2
POPD
PUSHD ts-microservice\openapi\unpkg-example
SET COMMAND=-JO -node:%NODEVERSION% %FILES%
echo %COMPILER% %COMMAND%
%COMPILER% %COMMAND%
echo.
if NOT %ERRORLEVEL% == 0 echo Build has failed, check console...>&2
POPD




echo finished...
GOTO end

:error
ECHO Could not find esnacc compiler executable in this folder %COMPILERPATH%
ECHO You need to build the compiler using cmake 
ECHO Check the top readme.md how to build the compiler

:end