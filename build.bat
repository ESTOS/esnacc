@echo off
REM Create build directories
if not exist build_win mkdir build_win
cd build_win

if not exist debug mkdir debug
if not exist release mkdir release

REM Debug build
cd debug
cmake -DCMAKE_BUILD_TYPE=Debug ..\..
cmake --build .
cd ..\..

REM Release build
cd release
cmake -DCMAKE_BUILD_TYPE=Release ..\..
cmake --build .

rem back to the roots
cd ..\..

REM Build completed
echo Build completed.
