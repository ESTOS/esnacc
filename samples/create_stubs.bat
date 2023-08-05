@echo off

rem This batch compiles the asn1 sample interface all the samples

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
PUSHD ts-client\src\stub
SET COMMAND=-JT -j -RTS_CLIENT_BROWSER %FILES%
echo %COMPILER% %COMMAND%
%COMPILER% %COMMAND%
echo.
if NOT %ERRORLEVEL% == 0 echo Build has failed, check console...>&2
POPD

rem Building node server stubs...
echo Building node server stubs...
PUSHD ts-server\src\stub
SET COMMAND=-JT -j -RTS_SERVER %FILES%
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