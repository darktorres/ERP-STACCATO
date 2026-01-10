FROM php:8.3-cli-alpine

# Install PostgreSQL dev libraries and PHP pgsql extension
RUN apk add --no-cache postgresql-dev \
    && docker-php-ext-install pdo pdo_pgsql pgsql

WORKDIR /app

# Copy application files
COPY schema.sql ./
COPY web/ ./web/

EXPOSE 8080

CMD ["php", "-S", "0.0.0.0:8080", "-t", "web"]
