@echo off
:: Deploy runtime dependencies next to a built .exe.
::
:: Usage:  tools\deploy.cmd <path-to-exe> <profile>
::   profile = "app"   → full app (Qt DLLs + plugins + 3rdparty DLLs)
::   profile = "test"  → test binary (Qt DLLs + plugins only)
::
:: Notes
:: - Designed to be invoked from qmake's QMAKE_POST_LINK in Loja.pro and
::   tests/tier1/tier1.pro. Re-runs cheaply: windeployqt is no-op when up
::   to date, and `copy /Y` is a fast file-stamp compare on NTFS.
:: - Does NOT copy `mysql.txt` / `lojas.txt` / `google_api.txt`. Those hold
::   secrets (DB password, API key, store hostnames) — they MUST be
::   provided manually in the .exe's working directory.
:: - Resolves the repo root from this script's own path; safe to invoke
::   from any working directory.

setlocal EnableDelayedExpansion

set "EXE=%~1"
set "PROFILE=%~2"

if "%EXE%"=="" (
    echo [deploy] usage: %~nx0 ^<path-to-exe^> ^<app^|test^>
    exit /b 2
)
if not exist "%EXE%" (
    echo [deploy] ERROR: exe not found: %EXE%
    exit /b 3
)

set "EXE_DIR=%~dp1"
set "EXE_DIR=%EXE_DIR:~0,-1%"
set "REPO_ROOT=%~dp0.."
for %%I in ("%REPO_ROOT%") do set "REPO_ROOT=%%~fI"

:: ---- windeployqt: Qt DLLs + platform plugin + style plugins ---------------
:: --debug picks debug Qt5 DLLs (Qt5Cored.dll etc). qmake's QMAKE_POST_LINK
:: fires for both debug and release — we infer build mode from the exe path.
set "DEPLOY_MODE=--debug"
echo %EXE% | findstr /I "\\release\\" >nul && set "DEPLOY_MODE=--release"

set "WINDEPLOYQT=C:\Qt\5.15.2\msvc2019\bin\windeployqt.exe"
if not exist "%WINDEPLOYQT%" (
    echo [deploy] WARNING: windeployqt not at %WINDEPLOYQT% — skipping Qt deploy
) else (
    echo [deploy] windeployqt %DEPLOY_MODE% %EXE%
    "%WINDEPLOYQT%" %DEPLOY_MODE% --no-translations --no-system-d3d-compiler --no-opengl-sw --compiler-runtime "%EXE%" >nul
    if errorlevel 1 (
        echo [deploy] ERROR: windeployqt failed
        exit /b 4
    )
)

:: ---- 3rdparty DLLs (app profile only) -------------------------------------
if /I "%PROFILE%"=="test" goto :done

call :copyfile "%REPO_ROOT%\3rdparty\cURL_x86-msvc\bin\libcurl.dll"        "%EXE_DIR%"
call :copyfile "%REPO_ROOT%\3rdparty\cURL_x86-msvc\bin\zlib1.dll"          "%EXE_DIR%"
call :copyfile "%REPO_ROOT%\3rdparty\OpenSSL-1.1-Win32\libcrypto-1_1.dll"  "%EXE_DIR%"
call :copyfile "%REPO_ROOT%\3rdparty\OpenSSL-1.1-Win32\libssl-1_1.dll"     "%EXE_DIR%"
call :copyfile "%REPO_ROOT%\3rdparty\MySQL_x86\bin\libmysql.dll"           "%EXE_DIR%"
call :copyfile "%REPO_ROOT%\3rdparty\ACBrLib\ACBrNFe32.dll"                "%EXE_DIR%"
call :copyfile "%REPO_ROOT%\xerces-c_3_3.dll"                              "%EXE_DIR%"

:done
echo [deploy] OK: %EXE%
exit /b 0

:: ---------------------------------------------------------------------------
:copyfile
:: Copy %~1 to %~2 if the source exists. Prints what was done.
if not exist "%~1" (
    echo [deploy] WARNING: missing source: %~1
    exit /b 0
)
copy /Y "%~1" "%~2" >nul
if errorlevel 1 (
    echo [deploy] ERROR: copy failed: %~1 -^> %~2
    exit /b 5
)
echo [deploy]   copied %~nx1
exit /b 0
