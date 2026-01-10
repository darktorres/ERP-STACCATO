#!/bin/bash
set -e

# Backup configuration
BACKUP_DIR="${BACKUP_DIR:-/backups}"
DB_HOST="${DB_HOST:-db}"
DB_PORT="${DB_PORT:-5432}"
DB_NAME="${DB_NAME:-staccato}"
DB_USERNAME="${DB_USERNAME:-staccato}"
RETENTION_DAYS="${RETENTION_DAYS:-30}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE="$BACKUP_DIR/staccato_${TIMESTAMP}.sql.gz"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Starting PostgreSQL backup...${NC}"

# Create backup directory if it doesn't exist
mkdir -p "$BACKUP_DIR"

# Perform backup
if pg_dump -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USERNAME" -d "$DB_NAME" 2>/dev/null | gzip > "$BACKUP_FILE"; then
    BACKUP_SIZE=$(du -h "$BACKUP_FILE" | cut -f1)
    echo -e "${GREEN}Backup created successfully: $BACKUP_FILE (Size: $BACKUP_SIZE)${NC}"
else
    echo -e "${RED}Backup failed!${NC}"
    exit 1
fi

# Remove old backups (retention policy)
echo -e "${YELLOW}Removing backups older than $RETENTION_DAYS days...${NC}"
find "$BACKUP_DIR" -name "staccato_*.sql.gz" -type f -mtime +$RETENTION_DAYS -delete
echo -e "${GREEN}Old backups removed${NC}"

# List current backups
echo -e "${YELLOW}Current backups:${NC}"
ls -lh "$BACKUP_DIR"/staccato_*.sql.gz 2>/dev/null | awk '{print $5, $9}' | sort -k2 -r || echo "No backups found"

echo -e "${GREEN}Backup process completed${NC}"
