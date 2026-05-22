@echo off
:: tools\build.cmd — one-shot build of staccato.pro (libstaccato + Loja +
:: every tier) using the project's pinned MSVC + Qt toolchain.
::
:: Usage:  tools\build.cmd [debug|release]
:: Default is debug. Run from the repo root.

setlocal

set "BUILD_MODE=%~1"
if "%BUILD_MODE%"=="" set "BUILD_MODE=debug"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64
if errorlevel 1 (
    echo VsDevCmd setup failed
    exit /b 1
)

set "PATH=C:\Qt\5.15.2\msvc2019\bin;%PATH%"

echo === qmake ===
qmake.exe staccato.pro
if errorlevel 1 exit /b 1

echo === nmake %BUILD_MODE% ===
nmake %BUILD_MODE%
exit /b %ERRORLEVEL%
