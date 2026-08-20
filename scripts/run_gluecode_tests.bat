@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
set "GLUE_DIR=%REPO_ROOT%\compiler\back-ends\ts-gen\gluecode"
set "TEST_DIR=%REPO_ROOT%\compiler\back-ends\ts-gen\tests"
set "STUB_DIR=%REPO_ROOT%\samples\ts-microservice\node-client\src\stub"
set "NODE_MODULES=%REPO_ROOT%\samples\ts-microservice\node-client\node_modules"

if not exist "%NODE_MODULES%\@estos\asn1ts" (
	echo error: run samples\prepare.bat first to install node-client dependencies. 1>&2
	exit /b 1
)

set "NODE_PATH=%NODE_MODULES%"
set "EXIT_CODE=0"

for %%F in (ENetUC_Common.ts ENetUC_Common_Converter.ts) do (
	if not exist "%GLUE_DIR%\%%F" (
		copy /Y "%STUB_DIR%\%%F" "%GLUE_DIR%\%%F" >nul
		set "COPIED_FIXTURE=1"
	)
)

for %%T in (
	TSASN1Base.registry.test.ts
	TSASN1Base.remoteCapability.test.ts
	TSModuleCapabilities.test.ts
) do (
	echo Running %%T ...
	npx --yes tsx "%TEST_DIR%\%%T"
	if errorlevel 1 set "EXIT_CODE=1"
)

if defined COPIED_FIXTURE (
	del /Q "%GLUE_DIR%\ENetUC_Common.ts" "%GLUE_DIR%\ENetUC_Common_Converter.ts" 2>nul
)

exit /b %EXIT_CODE%
