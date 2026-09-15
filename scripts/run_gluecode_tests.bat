@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
set "GLUE_DIR=%REPO_ROOT%\compiler\back-ends\ts-gen\gluecode"
set "TEST_DIR=%REPO_ROOT%\compiler\back-ends\ts-gen\tests"
set "WORKDIR=%TEST_DIR%\workdir"
set "STUB_DIR=%REPO_ROOT%\samples\ts-microservice\node-client\src\stub"
set "NODE_MODULES=%REPO_ROOT%\samples\ts-microservice\node-client\node_modules"

if not exist "%NODE_MODULES%\@estos\asn1ts" (
	echo error: run samples\prepare.bat first to install node-client dependencies. 1>&2
	exit /b 1
)

set "NODE_PATH=%NODE_MODULES%"
set "EXIT_CODE=0"

if exist "%TEST_DIR%\workdir" rmdir /s /q "%TEST_DIR%\workdir"
mkdir "%WORKDIR%"
xcopy /E /I /Y /Q "%GLUE_DIR%\*" "%WORKDIR%\" >nul
copy /Y "%STUB_DIR%\ENetUC_Common.ts" "%WORKDIR%\" >nul
copy /Y "%STUB_DIR%\ENetUC_Common_Converter.ts" "%WORKDIR%\" >nul
copy /Y "%STUB_DIR%\ENetUC_Settings_Manager.ts" "%WORKDIR%\" >nul
copy /Y "%STUB_DIR%\ENetUC_Settings_Manager_Converter.ts" "%WORKDIR%\" >nul
copy /Y "%STUB_DIR%\ENetUC_Settings_ManagerROSE.ts" "%WORKDIR%\" >nul
copy /Y "%STUB_DIR%\ENetUC_Settings_ManagerROSE_Interface.ts" "%WORKDIR%\" >nul
copy /Y "%STUB_DIR%\ENetUC_Event_Manager.ts" "%WORKDIR%\" >nul
copy /Y "%STUB_DIR%\ENetUC_Event_Manager_Converter.ts" "%WORKDIR%\" >nul
copy /Y "%STUB_DIR%\ENetUC_Event_ManagerROSE.ts" "%WORKDIR%\" >nul
copy /Y "%STUB_DIR%\ENetUC_Event_ManagerROSE_Interface.ts" "%WORKDIR%\" >nul

for %%T in (
	TSASN1Base.registry.test.ts
	TSASN1Base.invokeBlockPolicy.test.ts
	TSASN1Base.roseSessionSubscription.test.ts
	TSASN1Base.pauseRoseProcessing.test.ts
	TSModuleCapabilities.test.ts
	TSROSEBase.invokeTimeout.test.ts
	TSInvokeContext.init.test.ts
	TSCallFlow.loopback.test.ts
	TSLogicalFailure.loopback.test.ts
	TSTransportFailure.loopback.test.ts
) do (
	echo Running %%T ...
	set "NODE_PATH=%NODE_MODULES%"
	npx --yes tsx "%TEST_DIR%\%%T"
	if errorlevel 1 set "EXIT_CODE=1"
)

if exist "%TEST_DIR%\workdir" rmdir /s /q "%TEST_DIR%\workdir"

exit /b %EXIT_CODE%
