@echo off
setlocal EnableExtensions
node "%~dp0run-all-snacc-tests.mjs"
exit /b %ERRORLEVEL%
