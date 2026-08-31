@echo off
setlocal
cd /d "%~dp0"

if not exist "build\bin\Release\MineLongwallSimulation.exe" (
  echo Release executable not found. Run build.bat first.
  exit /b 1
)

"build\bin\Release\MineLongwallSimulation.exe" %*
endlocal
