@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Ensures the esnacc compiler is current and sets SNACC_COMPILER + PATH.
rem Pure CMD — no PowerShell or Node required. Linux/macOS: ensure_compiler.sh
rem
rem Environment:
rem   SNACCLIB7_ROOT              Override repository root (default: parent of scripts/)
rem   SNACC_CMAKE_BUILD_DIR       CMake build dir relative to repo root
rem   SNACC_CMAKE_GENERATOR       Optional CMake -G for first-time configure
rem   SNACC_CONFIGURATION         CMake build config (default: Release)
rem   SNACC_SKIP_COMPILER_BUILD=1 Resolve only; never run cmake --build
rem   SNACC_FORCE_COMPILER_BUILD=1 Bypass SNACC_COMPILER/CMAKE_COMPILER_TARGET and use CMake
rem   SNACC_COMPILER              If set and skip/force rules allow, use without building

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
if defined SNACCLIB7_ROOT (
	for %%I in ("%SNACCLIB7_ROOT%") do set "REPO_ROOT=%%~fI"
) else (
	for %%I in ("%SCRIPT_DIR%\..") do set "REPO_ROOT=%%~fI"
)

if not defined SNACC_CONFIGURATION set "SNACC_CONFIGURATION=Release"

if "%SNACC_SKIP_COMPILER_BUILD%"=="1" goto :skip_build
if defined SNACC_COMPILER if not "%SNACC_FORCE_COMPILER_BUILD%"=="1" goto :use_existing_env
if defined CMAKE_COMPILER_TARGET if not "%SNACC_FORCE_COMPILER_BUILD%"=="1" goto :use_existing_env
goto :ensure_build

:skip_build
call :ResolveExisting
if errorlevel 1 (
	echo SNACC_SKIP_COMPILER_BUILD is set but no esnacc compiler was found. 1>&2
	exit /b 1
)
set "COMPILER_PATH=!RESOLVE_RESULT!"
goto :set_env

:use_existing_env
call :ResolveExisting
if errorlevel 1 (
	echo SNACC_COMPILER/CMAKE_COMPILER_TARGET is set but the compiler was not found. 1>&2
	exit /b 1
)
set "COMPILER_PATH=!RESOLVE_RESULT!"
goto :set_env

:ensure_build
call :FindBuildDir
call :EnsureConfigured
if errorlevel 1 exit /b 1
call :CompilerFromCache
set "COMPILER_PATH=!COMPILER_RESULT!"

echo Building esnacc compiler (%SNACC_CONFIGURATION%) via CMake
cmake --build "!BUILD_DIR!" --config %SNACC_CONFIGURATION% --target compiler
if errorlevel 1 (
	echo cmake --build compiler failed 1>&2
	exit /b 1
)
call :CompilerFromCache
set "COMPILER_PATH=!COMPILER_RESULT!"

if not exist "!COMPILER_PATH!" (
	echo esnacc compiler not found at "!COMPILER_PATH!" 1>&2
	exit /b 1
)

:set_env
for %%I in ("!COMPILER_PATH!") do (
	endlocal
	set "SNACC_COMPILER=%%~fI"
	set "PATH=%%~dpI;%PATH%"
)
exit /b 0

rem ---------------------------------------------------------------------------
:FindBuildDir
if defined SNACC_CMAKE_BUILD_DIR (
	set "BUILD_DIR=!REPO_ROOT!\!SNACC_CMAKE_BUILD_DIR!"
	exit /b 0
)
for %%D in (
	build\x64_vc145
	build\win32_vc145
	build\release
	build\debug
	build_win\release
	build_win\debug
	build
) do (
	if exist "!REPO_ROOT!\%%D\CMakeCache.txt" (
		set "BUILD_DIR=!REPO_ROOT!\%%D"
		exit /b 0
	)
)
set "BUILD_DIR=!REPO_ROOT!\build"
exit /b 0

rem ---------------------------------------------------------------------------
:ReadCacheValue
set "CACHE_VALUE="
set "CACHE_FILE=%~1"
set "CACHE_KEY=%~2"
if not exist "!CACHE_FILE!" exit /b 1
for /f "usebackq tokens=1,* delims==" %%A in (`findstr /b /c:"!CACHE_KEY!:" "!CACHE_FILE!" 2^>nul`) do (
	set "CACHE_VALUE=%%B"
)
if not defined CACHE_VALUE exit /b 1
if "!CACHE_VALUE:~0,13!"=="UNINITIALIZED=" set "CACHE_VALUE=!CACHE_VALUE:~13!"
exit /b 0

rem ---------------------------------------------------------------------------
:CompilerFromCache
set "CACHE_FILE=!BUILD_DIR!\CMakeCache.txt"
call :ReadCacheValue "!CACHE_FILE!" COMPILER_OUTPUT_PATH
if errorlevel 1 (
	set "OUTPUT_DIR=!REPO_ROOT!\output\bin"
) else (
	set "OUTPUT_DIR=!CACHE_VALUE!"
)
call :ReadCacheValue "!CACHE_FILE!" COMPILER_OUTPUT_NAME
if errorlevel 1 (
	set "OUTPUT_NAME=esnacc"
) else (
	set "OUTPUT_NAME=!CACHE_VALUE!"
)
set "COMPILER_RESULT=!OUTPUT_DIR!\!OUTPUT_NAME!.exe"
exit /b 0

rem ---------------------------------------------------------------------------
:EnsureConfigured
set "CACHE_FILE=!BUILD_DIR!\CMakeCache.txt"
if exist "!CACHE_FILE!" exit /b 0

echo Configuring esnacc CMake build in !BUILD_DIR!
if not exist "!BUILD_DIR!" mkdir "!BUILD_DIR!"
set "OUTPUT_DIR=!REPO_ROOT!\output\bin"
if defined SNACC_CMAKE_GENERATOR (
	cmake -G "!SNACC_CMAKE_GENERATOR!" -S "!REPO_ROOT!" -B "!BUILD_DIR!" -A x64 -DMSVC_STATIC_RUNTIME=ON -DBUILD_TESTING=OFF -DCOMPILER_OUTPUT_PATH="!OUTPUT_DIR!" -DCOMPILER_OUTPUT_NAME=esnacc
) else (
	cmake -S "!REPO_ROOT!" -B "!BUILD_DIR!" -A x64 -DMSVC_STATIC_RUNTIME=ON -DBUILD_TESTING=OFF -DCOMPILER_OUTPUT_PATH="!OUTPUT_DIR!" -DCOMPILER_OUTPUT_NAME=esnacc
)
if errorlevel 1 (
	echo cmake configure failed 1>&2
	exit /b 1
)
exit /b 0

rem ---------------------------------------------------------------------------
:ResolveExisting
set "RESOLVE_RESULT="
for %%C in ("%SNACC_COMPILER%" "%ESNACC_EXECUTABLE%" "%CMAKE_COMPILER_TARGET%") do (
	if not "%%~C"=="" if exist "%%~C" (
		for %%I in ("%%~C") do set "RESOLVE_RESULT=%%~fI"
		exit /b 0
	)
)
for %%D in ("!REPO_ROOT!\output\bin" "!REPO_ROOT!\samples\bin") do (
	for %%N in (esnacc.exe esnaccd.exe esnacc7.exe esnacc7d.exe) do (
		if exist "%%~D\%%N" (
			for %%I in ("%%~D\%%N") do set "RESOLVE_RESULT=%%~fI"
			exit /b 0
		)
	)
)
for %%N in (esnacc.exe esnaccd.exe esnacc7.exe esnacc7d.exe) do (
	for /f "delims=" %%P in ('where %%N 2^>nul') do (
		for %%I in ("%%P") do set "RESOLVE_RESULT=%%~fI"
		exit /b 0
	)
)
exit /b 1
