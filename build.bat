@echo off
setlocal
cd /d "%~dp0"

python -m venv .venv-build
if errorlevel 1 exit /b 1
.venv-build\Scripts\python.exe -m pip install -r requirements-build.txt
if errorlevel 1 exit /b 1
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DMINE_BUILD_APP=ON -DMINE_BUILD_TESTS=ON -DPython_EXECUTABLE="%CD%\.venv-build\Scripts\python.exe"
if errorlevel 1 exit /b 1

cmake --build build --config Debug --parallel 2
if errorlevel 1 exit /b 1
ctest --test-dir build -C Debug --output-on-failure
if errorlevel 1 exit /b 1

cmake --build build --config Release --parallel 2
if errorlevel 1 exit /b 1
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 exit /b 1

echo Build and tests completed successfully.
endlocal
