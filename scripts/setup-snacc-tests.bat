@echo off
setlocal EnableExtensions
node "%~dp0setup-snacc-tests.mjs" %*
exit /b %ERRORLEVEL%
