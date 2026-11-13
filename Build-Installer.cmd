@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Installer\Build-Installer.ps1" %*
set "installerExit=%ERRORLEVEL%"
if not "%installerExit%"=="0" echo Installer build failed with exit code %installerExit%.
exit /b %installerExit%
