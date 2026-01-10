@echo off
setlocal enabledelayedexpansion

echo ======================================================================
echo   Next-Gen Schema Test (via Docker psql)
echo ======================================================================
echo.

set CONTAINER=erp_nextgen_postgres
set DB=erp_nextgen_test

REM Check if container is running
docker ps --filter "name=%CONTAINER%" --format "{{.Names}}" | findstr %CONTAINER% >nul
if errorlevel 1 (
    echo [ERROR] Container %CONTAINER% is not running!
    echo Run: docker-compose up -d
    exit /b 1
)

echo [OK] Container is running
echo.

REM Run schema
echo ======================================================================
echo   Setting up schema...
echo ======================================================================
docker exec -i %CONTAINER% psql -U postgres -d %DB% < schema.sql
if errorlevel 1 (
    echo [ERROR] Failed to create schema
    exit /b 1
)
echo [OK] Schema created
echo.

REM Run tests via SQL
echo ======================================================================
echo   Running tests...
echo ======================================================================
docker exec -i %CONTAINER% psql -U postgres -d %DB% < test.sql

echo.
echo [DONE] Tests completed!
