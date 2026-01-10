#!/bin/bash
set -e

# Healthcheck script for Docker HEALTHCHECK directive

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

HEALTH_STATUS="healthy"
CHECKS_FAILED=0

# Check if the application is responding
echo -e "${YELLOW}Checking application health...${NC}"
if curl -sf http://localhost/health > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Application is responding${NC}"
else
    echo -e "${RED}✗ Application health check failed${NC}"
    HEALTH_STATUS="unhealthy"
    CHECKS_FAILED=$((CHECKS_FAILED + 1))
fi

# Check PHP-FPM status
echo -e "${YELLOW}Checking PHP-FPM status...${NC}"
if curl -sf http://localhost/status > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PHP-FPM is running${NC}"
else
    echo -e "${RED}✗ PHP-FPM status check failed${NC}"
    CHECKS_FAILED=$((CHECKS_FAILED + 1))
fi

# Check PHP-FPM ping
echo -e "${YELLOW}Checking PHP-FPM ping...${NC}"
if curl -sf http://localhost/ping > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PHP-FPM ping successful${NC}"
else
    echo -e "${RED}✗ PHP-FPM ping failed${NC}"
    CHECKS_FAILED=$((CHECKS_FAILED + 1))
fi

# Check database connectivity
echo -e "${YELLOW}Checking database connectivity...${NC}"
if pg_isready -h "${DB_HOST:-db}" -p "${DB_PORT:-5432}" -U "${DB_USERNAME:-staccato}" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PostgreSQL is reachable${NC}"
else
    echo -e "${RED}✗ PostgreSQL is not reachable${NC}"
    CHECKS_FAILED=$((CHECKS_FAILED + 1))
fi

# Check Redis connectivity
echo -e "${YELLOW}Checking Redis connectivity...${NC}"
if redis-cli -h "${REDIS_HOST:-redis}" -p "${REDIS_PORT:-6379}" ping > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Redis is reachable${NC}"
else
    echo -e "${RED}✗ Redis is not reachable${NC}"
    CHECKS_FAILED=$((CHECKS_FAILED + 1))
fi

# Summary
echo ""
if [ $CHECKS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All health checks passed${NC}"
    exit 0
else
    echo -e "${RED}$CHECKS_FAILED health check(s) failed${NC}"
    exit 1
fi
