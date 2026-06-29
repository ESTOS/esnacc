@echo off
node "%~dp0prepare.js" %*
set EXIT_CODE=%ERRORLEVEL%
if defined SNACC_NO_PAUSE exit /b %EXIT_CODE%
echo.
if %EXIT_CODE% NEQ 0 (
	echo Prepare failed with exit code %EXIT_CODE%.
	pause
) else (
	echo Prepare finished. Closing in 10 seconds...
	timeout /t 10
)
exit /b %EXIT_CODE%
