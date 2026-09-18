@echo off
setlocal EnableExtensions
node "%~dp0setup-ts-glue-tests.mjs" %*
exit /b %ERRORLEVEL%
node "%~dp0setup-snacc-tests.mjs" --ts-only %*