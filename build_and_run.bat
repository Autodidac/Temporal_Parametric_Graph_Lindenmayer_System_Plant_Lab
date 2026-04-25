@echo off
setlocal enabledelayedexpansion

set "ROOT=%~dp0."
set "BUILD=%ROOT%\build\vs2022-viewer-release"
set "TRIPLET=x64-windows"

if not defined VCPKG_ROOT (
    echo [TwoOL] ERROR: VCPKG_ROOT is not set.
    echo [TwoOL] Set it to your vcpkg folder, for example:
    echo [TwoOL]   setx VCPKG_ROOT C:\Users\iammi\source\repos\vcpkg
    exit /b 1
)

if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo [TwoOL] ERROR: vcpkg.exe was not found at VCPKG_ROOT: %VCPKG_ROOT%
    exit /b 1
)

echo [TwoOL] Installing/checking glfw3:%TRIPLET%...
"%VCPKG_ROOT%\vcpkg.exe" install glfw3:%TRIPLET%
if errorlevel 1 (
    echo [TwoOL] ERROR: vcpkg install failed.
    exit /b 1
)

echo [TwoOL] Configuring OpenGL viewer...
cmake -S "%ROOT%" -B "%BUILD%" -G "Visual Studio 17 2022" -A x64 -DTWOL_BUILD_HEADLESS_DUMP=ON -DTWOL_BUILD_OPENGL_VIEWER=ON -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if errorlevel 1 (
    echo [TwoOL] ERROR: CMake configure failed.
    exit /b 1
)

echo [TwoOL] Building OpenGL viewer...
cmake --build "%BUILD%" --config Release --parallel
if errorlevel 1 (
    echo [TwoOL] ERROR: Build failed.
    exit /b 1
)

echo [TwoOL] Running viewer...
"%BUILD%\Release\twol_plant_viewer.exe"
exit /b %ERRORLEVEL%
