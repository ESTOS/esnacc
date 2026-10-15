@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
set "NODE_MODULES=%REPO_ROOT%\samples\ts-microservice\node-client\node_modules"

if not exist "%NODE_MODULES%\@estos\asn1ts" (
	echo error: run scripts\setup-snacc-tests.bat first. 1>&2
	exit /b 1
)

node "%REPO_ROOT%\scripts\run-ts-glue-test-suite.mjs"
exit /b %ERRORLEVEL%
