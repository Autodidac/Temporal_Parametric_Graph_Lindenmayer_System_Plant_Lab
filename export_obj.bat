@echo off
setlocal enabledelayedexpansion

set "ROOT=%~dp0."
set "BUILD=%ROOT%\build\vs2022-headless-release"
set "OUT=%ROOT%\out"

if not exist "%OUT%" mkdir "%OUT%"

echo [TwoOL] Configuring headless exporter...
cmake -S "%ROOT%" -B "%BUILD%" -G "Visual Studio 17 2022" -A x64 -DTWOL_BUILD_HEADLESS_DUMP=ON -DTWOL_BUILD_OPENGL_VIEWER=OFF
if errorlevel 1 (
    echo [TwoOL] ERROR: CMake configure failed.
    exit /b 1
)

echo [TwoOL] Building headless exporter...
cmake --build "%BUILD%" --config Release --parallel
if errorlevel 1 (
    echo [TwoOL] ERROR: Build failed.
    exit /b 1
)

echo [TwoOL] Exporting OBJ/TWOL...
"%BUILD%\Release\twol_headless_dump.exe" "%OUT%"
exit /b %ERRORLEVEL%
