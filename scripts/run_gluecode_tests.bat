@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
set "TEST_DIR=%REPO_ROOT%\compiler\tests\ts"
set "NODE_MODULES=%REPO_ROOT%\samples\ts-microservice\node-client\node_modules"

if not exist "%NODE_MODULES%\@estos\asn1ts" (
	echo error: run scripts\setup-snacc-tests.bat first. 1>&2
	exit /b 1
)

set "EXIT_CODE=0"

node "%REPO_ROOT%\scripts\prepare-ts-glue-stub.mjs" --clean
if errorlevel 1 exit /b 1

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
	node "%REPO_ROOT%\scripts\run-ts-glue-test-file.mjs" "%%T"
	if errorlevel 1 set "EXIT_CODE=1"
)

if exist "%TEST_DIR%\stub" rmdir /s /q "%TEST_DIR%\stub"

exit /b %EXIT_CODE%
