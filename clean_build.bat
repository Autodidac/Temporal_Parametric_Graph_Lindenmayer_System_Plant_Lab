@echo off
setlocal
set "ROOT=%~dp0."
if exist "%ROOT%\build" rmdir /s /q "%ROOT%\build"
if exist "%ROOT%\out" rmdir /s /q "%ROOT%\out"
echo [TwoOL] Cleaned build/ and out/.
