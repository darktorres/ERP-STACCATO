# ==========================================
# Stage 1: Build Frontend Assets
# ==========================================
FROM node:24-alpine AS frontend

WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci --prefer-offline

COPY resources/ resources/
COPY vite.config.js tailwind.config.js postcss.config.js ./
COPY tsconfig.json ./

RUN npm run build

# ==========================================
# Stage 2: Composer Dependencies
# ==========================================
FROM composer:2.9 AS composer

WORKDIR /app

COPY composer.json composer.lock ./

RUN composer install \
    --no-dev \
    --no-scripts \
    --no-autoloader \
    --prefer-dist \
    --ignore-platform-reqs

COPY . .
RUN composer dump-autoload --optimize --no-dev

# ==========================================
# Stage 3: Production Image
# ==========================================
FROM php:8.5-fpm-alpine AS production

ARG APP_VERSION=1.0.0
ARG BUILD_DATE
ARG VCS_REF

LABEL org.opencontainers.image.title="ERP Staccato" \
      org.opencontainers.image.version="${APP_VERSION}" \
      org.opencontainers.image.created="${BUILD_DATE}" \
      org.opencontainers.image.revision="${VCS_REF}" \
      org.opencontainers.image.vendor="Staccato"

RUN apk add --no-cache \
    nginx \
    supervisor \
    curl \
    libpng \
    libjpeg-turbo \
    freetype \
    libzip \
    icu \
    libpq \
    && apk add --no-cache --virtual .build-deps \
    $PHPIZE_DEPS \
    libpng-dev \
    libjpeg-turbo-dev \
    freetype-dev \
    libzip-dev \
    icu-dev \
    postgresql-dev \
    linux-headers

RUN docker-php-ext-configure gd --with-freetype --with-jpeg \
    && docker-php-ext-install -j$(nproc) \
    pdo_pgsql \
    pgsql \
    gd \
    zip \
    intl \
    opcache \
    pcntl \
    bcmath

RUN pecl install redis \
    && docker-php-ext-enable redis

RUN apk del .build-deps

COPY docker/php/php.ini /usr/local/etc/php/php.ini
COPY docker/php/php-fpm.conf /usr/local/etc/php-fpm.conf
COPY docker/php/www.conf /usr/local/etc/php-fpm.d/www.conf
COPY docker/nginx/nginx.conf /etc/nginx/nginx.conf
COPY docker/nginx/sites/default.conf /etc/nginx/http.d/default.conf
COPY docker/supervisord/supervisord.conf /etc/supervisor/conf.d/supervisord.conf

RUN mkdir -p /var/log/supervisor \
    && mkdir -p /var/run/nginx \
    && mkdir -p /var/cache/nginx

WORKDIR /var/www/html

COPY --from=composer /app/vendor vendor/
COPY --from=frontend /app/public/build public/build/
COPY . .

RUN mkdir -p storage/{app,framework/{sessions,views,cache},logs} \
    && mkdir -p bootstrap/cache \
    && chown -R www-data:www-data storage bootstrap/cache \
    && chmod -R 775 storage bootstrap/cache

COPY docker/scripts/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -f http://localhost/health || exit 1

EXPOSE 80

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]
