@echo off
echo Starting Next-Gen Schema Web Demo...
echo.

cd /d "%~dp0"

REM Check if containers are running
docker ps --filter "name=erp_nextgen_postgres" --format "{{.Names}}" | findstr erp_nextgen_postgres >nul
if errorlevel 1 (
    echo Starting containers...
    docker-compose up -d
    echo Waiting for services to start...
    timeout /t 8 /nobreak >nul
)

echo.
echo =============================================
echo   Web Demo running at: http://localhost:8080
echo =============================================
echo.
echo Opening browser...
start http://localhost:8080

echo Press any key to stop the demo...
pause >nul

echo Stopping containers...
docker-compose down
