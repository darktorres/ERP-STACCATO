@echo off
:: tests\run-all.cmd — invoke every tier binary in order and report a summary.
::
:: Usage:  tests\run-all.cmd
::
:: Reads from current env (set STACCATO_TEST_DB_USER and *_PASS before
:: running, or rely on the tier2/3 binaries' SKIP-on-no-DB fallback).
:: Builds the binaries first if they don't exist. Idempotent: schema +
:: fixtures load only on first run against an empty MySQL.
::
:: Exit code: 0 if every tier passed, non-zero if any failed.

setlocal EnableDelayedExpansion

set "REPO=%~dp0.."
for %%I in ("%REPO%") do set "REPO=%%~fI"

set "TIER1_EXE=%REPO%\tests\tier1\debug\tier1_tests.exe"
set "TIER2_EXE=%REPO%\tests\tier2\debug\tier2_tests.exe"
set "TIER3_EXE=%REPO%\tests\tier3\debug\tier3_tests.exe"

set FAIL_COUNT=0

call :run_tier "tier1" "%TIER1_EXE%" || set /a FAIL_COUNT+=1
call :run_tier "tier2" "%TIER2_EXE%" || set /a FAIL_COUNT+=1
call :run_tier "tier3" "%TIER3_EXE%" || set /a FAIL_COUNT+=1

echo.
echo ============================================================
if %FAIL_COUNT% EQU 0 (
    echo ALL TIERS GREEN
    exit /b 0
) else (
    echo !FAIL_COUNT! TIER^(S^) FAILED
    exit /b !FAIL_COUNT!
)

:run_tier
:: %1 = label,  %2 = absolute exe path
set "LABEL=%~1"
set "EXE=%~2"
echo.
echo === %LABEL% ===
if not exist "%EXE%" (
    echo [%LABEL%] missing: %EXE%
    echo [%LABEL%] run  qmake staccato.pro ^&^& nmake debug  to build first
    exit /b 1
)
"%EXE%"
if errorlevel 1 (
    echo [%LABEL%] FAILED with exit %ERRORLEVEL%
    exit /b 1
)
exit /b 0
