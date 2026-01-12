@echo off

SET OLDPATH=%PATH%
where cmake >NUL 2>NUL
IF %ERRORLEVEL% == 0 GOTO build

SET PATH=%PATH%;C:\Program Files\CMake\bin\
:: VS2026
SET PATH=%PATH%;C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\
SET PATH=%PATH%;C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\
SET PATH=%PATH%;C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\
:: VS2022
SET PATH=%PATH%;C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\
SET PATH=%PATH%;C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\
SET PATH=%PATH%;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\
:: VS2019
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\

where cmake >NUL 2>NUL
IF %ERRORLEVEL% == 0 GOTO build

SET PATH=%OLDPATH%
echo Could not find cmake in PATH
echo Ensure that cmake is in PATH before calling this script
timeout /t 5
exit 1

:build

REM Create build directories
if not exist build_win mkdir build_win
pushd build_win

set COMMANDLINE=-DCMAKE_GENERATOR_PLATFORM=x64

REM Debug build
if not exist debug mkdir debug
pushd debug
cmake %COMMANDLINE% -DCMAKE_BUILD_TYPE=debug ..\..
cmake --build . --config debug
popd

REM Release build
if not exist release mkdir release
pushd release
cmake %COMMANDLINE% -DCMAKE_BUILD_TYPE=release ..\..
cmake --build . --config release
popd

rem back to the roots
popd

SET PATH=%OLDPATH%

REM Build completed
echo Build completed.
