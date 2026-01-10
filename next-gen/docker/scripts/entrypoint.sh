#!/bin/bash
set -e

echo "Starting application entrypoint..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Wait for PostgreSQL
echo -e "${YELLOW}Waiting for PostgreSQL...${NC}"
max_attempts=30
attempt=0
while ! pg_isready -h ${DB_HOST:-db} -p ${DB_PORT:-5432} -U ${DB_USERNAME:-staccato} > /dev/null 2>&1; do
    attempt=$((attempt + 1))
    if [ $attempt -gt $max_attempts ]; then
        echo -e "${RED}PostgreSQL not ready after $max_attempts attempts${NC}"
        exit 1
    fi
    echo "Attempt $attempt/$max_attempts: PostgreSQL not ready, waiting..."
    sleep 2
done
echo -e "${GREEN}PostgreSQL is ready!${NC}"

# Wait for Redis
echo -e "${YELLOW}Waiting for Redis...${NC}"
attempt=0
while ! redis-cli -h ${REDIS_HOST:-redis} -p ${REDIS_PORT:-6379} ping > /dev/null 2>&1; do
    attempt=$((attempt + 1))
    if [ $attempt -gt $max_attempts ]; then
        echo -e "${RED}Redis not ready after $max_attempts attempts${NC}"
        exit 1
    fi
    echo "Attempt $attempt/$max_attempts: Redis not ready, waiting..."
    sleep 2
done
echo -e "${GREEN}Redis is ready!${NC}"

# Create required directories
echo -e "${YELLOW}Creating required directories...${NC}"
mkdir -p /var/www/html/storage/framework/{sessions,views,cache}
mkdir -p /var/www/html/storage/logs
mkdir -p /var/www/html/bootstrap/cache
mkdir -p /var/log/php-fpm
mkdir -p /var/log/xdebug
mkdir -p /var/log/nginx
mkdir -p /var/log/supervisor

# Set permissions
echo -e "${YELLOW}Setting file permissions...${NC}"
chown -R www-data:www-data /var/www/html/storage /var/www/html/bootstrap/cache
chmod -R 775 /var/www/html/storage /var/www/html/bootstrap/cache
chmod -R 755 /var/log/php-fpm /var/log/xdebug /var/log/nginx /var/log/supervisor

# Run database migrations
if [ "$RUN_MIGRATIONS" = "true" ] || [ -z "$RUN_MIGRATIONS" ]; then
    echo -e "${YELLOW}Running database migrations...${NC}"
    php artisan migrate --force --no-interaction || {
        echo -e "${RED}Migrations failed${NC}"
        exit 1
    }
    echo -e "${GREEN}Migrations completed!${NC}"
fi

# Cache configurations (production mode)
if [ "$APP_ENV" = "production" ]; then
    echo -e "${YELLOW}Caching configurations for production...${NC}"
    php artisan config:cache --no-interaction
    php artisan route:cache --no-interaction
    php artisan view:cache --no-interaction
    php artisan event:cache --no-interaction
    echo -e "${GREEN}Configurations cached!${NC}"
fi

# Clear cache in development
if [ "$APP_ENV" = "local" ]; then
    echo -e "${YELLOW}Clearing caches in development mode...${NC}"
    php artisan cache:clear
    php artisan view:clear
fi

echo -e "${GREEN}Application entrypoint completed successfully!${NC}"

# Execute the command passed as arguments
exec "$@"
