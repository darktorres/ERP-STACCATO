# DevOps e Deployment

> Status: **Aprovado**
> Última atualização: 2025-12-28

---

## Visão Geral

Este documento define a estratégia de DevOps e deployment para a migração do ERP Staccato, convertendo de uma aplicação desktop Qt C++ com auto-update para uma aplicação web Laravel com deploy containerizado.

### Arquitetura Atual (C++)

```
┌─────────────────────────────────────────────────────────────┐
│                    Desktop (C++ Qt)                          │
├─────────────────────────────────────────────────────────────┤
│  Instalador.exe → Máquina Local → Conexão MySQL Remota      │
│                                                              │
│  Auto-Update: QSimpleUpdater                                 │
│  - versao.txt (HTTP)                                         │
│  - Instalador.exe (HTTP download)                            │
│                                                              │
│  Configuração Local:                                         │
│  - lojas.txt (servidores)                                    │
│  - mysql.txt (senha sistema)                                 │
│  - google_api.txt (API key)                                  │
└─────────────────────────────────────────────────────────────┘
```

### Arquitetura Nova (Laravel)

```
┌─────────────────────────────────────────────────────────────┐
│                    Web (Laravel + Inertia)                   │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────┐     ┌─────────┐     ┌─────────────────────┐   │
│  │ Browser │────▶│  Nginx  │────▶│  PHP-FPM (Laravel)  │   │
│  └─────────┘     └─────────┘     └─────────────────────┘   │
│                       │                    │                 │
│                       │              ┌─────▼─────┐          │
│                       │              │   Redis   │          │
│                       │              └───────────┘          │
│                       │                    │                 │
│                  ┌────▼────────────────────▼────┐           │
│                  │      PostgreSQL 16           │           │
│                  └──────────────────────────────┘           │
│                                                              │
│  Deployment: Docker → GitHub Actions → VPS/Cloud            │
│  SSL: Let's Encrypt (auto-renew)                            │
└─────────────────────────────────────────────────────────────┘
```

---

## Stack de Infraestrutura

### Componentes Principais

| Componente | Tecnologia | Versão |
|------------|------------|--------|
| Web Server | Nginx | 1.25+ |
| PHP Runtime | PHP-FPM | 8.3+ |
| Framework | Laravel | 12.x |
| Database | PostgreSQL | 16+ |
| Cache/Queue | Redis | 7+ |
| Container | Docker | 24+ |
| Orquestração | Docker Compose | 2.20+ |
| CI/CD | GitHub Actions | - |
| SSL | Let's Encrypt | - |
| Monitoramento | Sentry + Laravel Pulse | - |

### Requisitos de Servidor

**Mínimo (Produção Inicial):**
- 4 vCPUs
- 8 GB RAM
- 100 GB SSD
- Rede: 1 Gbps

**Recomendado (Produção Escalada):**
- 8 vCPUs
- 16 GB RAM
- 200 GB SSD NVMe
- Rede: 10 Gbps

---

## Configuração Docker

### Estrutura de Arquivos

```
docker/
├── Dockerfile                 # Multi-stage build
├── Dockerfile.dev             # Desenvolvimento
├── docker-compose.yml         # Produção
├── docker-compose.dev.yml     # Desenvolvimento
├── docker-compose.test.yml    # Testes
├── nginx/
│   ├── nginx.conf
│   └── sites/
│       └── default.conf
├── php/
│   ├── php.ini
│   ├── php-fpm.conf
│   └── www.conf
├── supervisord/
│   └── supervisord.conf
└── scripts/
    ├── entrypoint.sh
    ├── start-app.sh
    ├── start-queue.sh
    └── healthcheck.sh
```

### Dockerfile Multi-Stage

```dockerfile
# ==========================================
# Stage 1: Build Frontend Assets
# ==========================================
FROM node:20-alpine AS frontend

WORKDIR /app

# Copiar arquivos de dependência
COPY package.json package-lock.json ./

# Instalar dependências
RUN npm ci --prefer-offline

# Copiar código fonte
COPY resources/ resources/
COPY vite.config.js tailwind.config.js postcss.config.js ./
COPY tsconfig.json ./

# Build de produção
RUN npm run build

# ==========================================
# Stage 2: Composer Dependencies
# ==========================================
FROM composer:2.6 AS composer

WORKDIR /app

# Copiar composer files
COPY composer.json composer.lock ./

# Instalar dependências sem dev
RUN composer install \
    --no-dev \
    --no-scripts \
    --no-autoloader \
    --prefer-dist \
    --ignore-platform-reqs

# Copiar código para autoload
COPY . .

# Gerar autoload otimizado
RUN composer dump-autoload --optimize --no-dev

# ==========================================
# Stage 3: Production Image
# ==========================================
FROM php:8.3-fpm-alpine AS production

# Argumentos de build
ARG APP_VERSION=1.0.0
ARG BUILD_DATE
ARG VCS_REF

# Labels
LABEL org.opencontainers.image.title="ERP Staccato" \
      org.opencontainers.image.version="${APP_VERSION}" \
      org.opencontainers.image.created="${BUILD_DATE}" \
      org.opencontainers.image.revision="${VCS_REF}" \
      org.opencontainers.image.vendor="Staccato"

# Instalar dependências do sistema
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

# Instalar extensões PHP
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

# Instalar Redis
RUN pecl install redis \
    && docker-php-ext-enable redis

# Limpar build deps
RUN apk del .build-deps

# Copiar configurações
COPY docker/php/php.ini /usr/local/etc/php/php.ini
COPY docker/php/php-fpm.conf /usr/local/etc/php-fpm.conf
COPY docker/php/www.conf /usr/local/etc/php-fpm.d/www.conf
COPY docker/nginx/nginx.conf /etc/nginx/nginx.conf
COPY docker/nginx/sites/default.conf /etc/nginx/http.d/default.conf
COPY docker/supervisord/supervisord.conf /etc/supervisor/conf.d/supervisord.conf

# Criar diretórios necessários
RUN mkdir -p /var/log/supervisor \
    && mkdir -p /var/run/nginx \
    && mkdir -p /var/cache/nginx

# Definir workdir
WORKDIR /var/www/html

# Copiar código da aplicação
COPY --from=composer /app/vendor vendor/
COPY --from=frontend /app/public/build public/build/
COPY . .

# Configurar permissões
RUN chown -R www-data:www-data storage bootstrap/cache \
    && chmod -R 775 storage bootstrap/cache

# Copiar e configurar entrypoint
COPY docker/scripts/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Health check
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -f http://localhost/health || exit 1

# Expor porta
EXPOSE 80

# Entrypoint
ENTRYPOINT ["/entrypoint.sh"]

# Comando padrão
CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]
```

### docker-compose.yml (Produção)

```yaml
version: '3.8'

services:
  app:
    build:
      context: .
      dockerfile: docker/Dockerfile
      args:
        APP_VERSION: ${APP_VERSION:-1.0.0}
        BUILD_DATE: ${BUILD_DATE:-}
        VCS_REF: ${VCS_REF:-}
    image: staccato/erp:${APP_VERSION:-latest}
    container_name: staccato_app
    restart: unless-stopped
    ports:
      - "80:80"
      - "443:443"
    environment:
      - APP_ENV=production
      - APP_DEBUG=false
      - APP_URL=${APP_URL}
      - DB_CONNECTION=pgsql
      - DB_HOST=db
      - DB_PORT=5432
      - DB_DATABASE=${DB_DATABASE}
      - DB_USERNAME=${DB_USERNAME}
      - DB_PASSWORD=${DB_PASSWORD}
      - REDIS_HOST=redis
      - REDIS_PORT=6379
      - CACHE_DRIVER=redis
      - QUEUE_CONNECTION=redis
      - SESSION_DRIVER=redis
    volumes:
      - storage_logs:/var/www/html/storage/logs
      - storage_app:/var/www/html/storage/app
      - ssl_certs:/etc/letsencrypt
    depends_on:
      db:
        condition: service_healthy
      redis:
        condition: service_started
    networks:
      - staccato_network
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost/health"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 40s

  queue:
    build:
      context: .
      dockerfile: docker/Dockerfile
    image: staccato/erp:${APP_VERSION:-latest}
    container_name: staccato_queue
    restart: unless-stopped
    command: php artisan queue:work redis --sleep=3 --tries=3 --max-time=3600
    environment:
      - APP_ENV=production
      - DB_HOST=db
      - REDIS_HOST=redis
    depends_on:
      - app
      - redis
    networks:
      - staccato_network

  scheduler:
    build:
      context: .
      dockerfile: docker/Dockerfile
    image: staccato/erp:${APP_VERSION:-latest}
    container_name: staccato_scheduler
    restart: unless-stopped
    command: php artisan schedule:work
    environment:
      - APP_ENV=production
      - DB_HOST=db
      - REDIS_HOST=redis
    depends_on:
      - app
    networks:
      - staccato_network

  db:
    image: postgres:16-alpine
    container_name: staccato_db
    restart: unless-stopped
    environment:
      - POSTGRES_DB=${DB_DATABASE}
      - POSTGRES_USER=${DB_USERNAME}
      - POSTGRES_PASSWORD=${DB_PASSWORD}
    volumes:
      - postgres_data:/var/lib/postgresql/data
      - ./docker/postgres/init:/docker-entrypoint-initdb.d
    ports:
      - "5432:5432"
    networks:
      - staccato_network
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U ${DB_USERNAME} -d ${DB_DATABASE}"]
      interval: 10s
      timeout: 5s
      retries: 5

  redis:
    image: redis:7-alpine
    container_name: staccato_redis
    restart: unless-stopped
    command: redis-server --appendonly yes --maxmemory 256mb --maxmemory-policy allkeys-lru
    volumes:
      - redis_data:/data
    ports:
      - "6379:6379"
    networks:
      - staccato_network

  # Backup automático
  backup:
    image: postgres:16-alpine
    container_name: staccato_backup
    restart: unless-stopped
    environment:
      - PGHOST=db
      - PGUSER=${DB_USERNAME}
      - PGPASSWORD=${DB_PASSWORD}
      - PGDATABASE=${DB_DATABASE}
    volumes:
      - backups:/backups
      - ./docker/scripts/backup.sh:/backup.sh
    command: >
      sh -c "while true; do
        /backup.sh;
        sleep 86400;
      done"
    depends_on:
      - db
    networks:
      - staccato_network

volumes:
  postgres_data:
  redis_data:
  storage_logs:
  storage_app:
  ssl_certs:
  backups:

networks:
  staccato_network:
    driver: bridge
```

### docker-compose.dev.yml (Desenvolvimento)

```yaml
version: '3.8'

services:
  app:
    build:
      context: .
      dockerfile: docker/Dockerfile.dev
    container_name: staccato_dev
    ports:
      - "8000:80"
      - "5173:5173"  # Vite HMR
    environment:
      - APP_ENV=local
      - APP_DEBUG=true
      - DB_HOST=db
      - REDIS_HOST=redis
    volumes:
      - .:/var/www/html
      - /var/www/html/vendor
      - /var/www/html/node_modules
    depends_on:
      - db
      - redis
    networks:
      - staccato_dev

  db:
    image: postgres:16-alpine
    container_name: staccato_dev_db
    environment:
      - POSTGRES_DB=staccato_dev
      - POSTGRES_USER=staccato
      - POSTGRES_PASSWORD=secret
    volumes:
      - dev_postgres:/var/lib/postgresql/data
    ports:
      - "5433:5432"
    networks:
      - staccato_dev

  redis:
    image: redis:7-alpine
    container_name: staccato_dev_redis
    ports:
      - "6380:6379"
    networks:
      - staccato_dev

  mailpit:
    image: axllent/mailpit
    container_name: staccato_mailpit
    ports:
      - "8025:8025"
      - "1025:1025"
    networks:
      - staccato_dev

volumes:
  dev_postgres:

networks:
  staccato_dev:
    driver: bridge
```

---

## Configurações PHP

### php.ini (Produção)

```ini
[PHP]
; Configurações gerais
memory_limit = 256M
max_execution_time = 60
max_input_time = 60
post_max_size = 100M
upload_max_filesize = 50M
max_file_uploads = 20

; Timezone
date.timezone = America/Sao_Paulo

; Error handling
display_errors = Off
display_startup_errors = Off
log_errors = On
error_log = /var/log/php/error.log
error_reporting = E_ALL & ~E_DEPRECATED & ~E_STRICT

; Sessão
session.save_handler = redis
session.save_path = "tcp://redis:6379"
session.cookie_secure = On
session.cookie_httponly = On
session.cookie_samesite = Strict

; OPcache
opcache.enable = 1
opcache.enable_cli = 1
opcache.memory_consumption = 256
opcache.interned_strings_buffer = 32
opcache.max_accelerated_files = 20000
opcache.validate_timestamps = 0
opcache.revalidate_freq = 0
opcache.save_comments = 1
opcache.jit_buffer_size = 100M
opcache.jit = 1255

; Realpath cache
realpath_cache_size = 4096K
realpath_cache_ttl = 600

; Segurança
expose_php = Off
allow_url_fopen = Off
allow_url_include = Off
disable_functions = exec,passthru,shell_exec,system,proc_open,popen
```

### www.conf (PHP-FPM)

```ini
[www]
user = www-data
group = www-data

listen = /run/php-fpm.sock
listen.owner = www-data
listen.group = www-data
listen.mode = 0660

pm = dynamic
pm.max_children = 50
pm.start_servers = 10
pm.min_spare_servers = 5
pm.max_spare_servers = 20
pm.max_requests = 1000

; Status e health
pm.status_path = /status
ping.path = /ping
ping.response = pong

; Slow log
slowlog = /var/log/php-fpm/slow.log
request_slowlog_timeout = 10s

; Limites
request_terminate_timeout = 60s
rlimit_files = 131072
rlimit_core = unlimited

; Environment
clear_env = no
catch_workers_output = yes
decorate_workers_output = no
```

---

## Configuração Nginx

### nginx.conf

```nginx
user www-data;
worker_processes auto;
pid /run/nginx.pid;
error_log /var/log/nginx/error.log warn;

events {
    worker_connections 4096;
    multi_accept on;
    use epoll;
}

http {
    # Básico
    include /etc/nginx/mime.types;
    default_type application/octet-stream;

    # Logging
    log_format main '$remote_addr - $remote_user [$time_local] "$request" '
                    '$status $body_bytes_sent "$http_referer" '
                    '"$http_user_agent" "$http_x_forwarded_for" '
                    'rt=$request_time uct="$upstream_connect_time" '
                    'uht="$upstream_header_time" urt="$upstream_response_time"';

    access_log /var/log/nginx/access.log main buffer=16k flush=2s;

    # Performance
    sendfile on;
    tcp_nopush on;
    tcp_nodelay on;
    keepalive_timeout 65;
    types_hash_max_size 2048;
    server_tokens off;

    # Gzip
    gzip on;
    gzip_vary on;
    gzip_proxied any;
    gzip_comp_level 6;
    gzip_types text/plain text/css text/xml application/json application/javascript
               application/rss+xml application/atom+xml image/svg+xml;

    # Rate limiting
    limit_req_zone $binary_remote_addr zone=api:10m rate=60r/m;
    limit_req_zone $binary_remote_addr zone=login:10m rate=10r/m;

    # Uploads
    client_max_body_size 100M;
    client_body_buffer_size 128k;

    # Timeouts
    proxy_connect_timeout 60s;
    proxy_send_timeout 60s;
    proxy_read_timeout 60s;

    # Includes
    include /etc/nginx/http.d/*.conf;
}
```

### default.conf (Site)

```nginx
upstream php-fpm {
    server unix:/run/php-fpm.sock;
}

server {
    listen 80;
    listen [::]:80;
    server_name _;

    # Redirect HTTP to HTTPS (quando SSL configurado)
    # return 301 https://$server_name$request_uri;

    root /var/www/html/public;
    index index.php;

    charset utf-8;

    # Security headers
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-XSS-Protection "1; mode=block" always;
    add_header Referrer-Policy "strict-origin-when-cross-origin" always;

    # Health check
    location /health {
        access_log off;
        return 200 'OK';
        add_header Content-Type text/plain;
    }

    # Status do PHP-FPM (apenas interno)
    location ~ ^/(status|ping)$ {
        allow 127.0.0.1;
        deny all;
        fastcgi_pass php-fpm;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
        include fastcgi_params;
    }

    # Arquivos estáticos
    location ~* \.(jpg|jpeg|png|gif|ico|css|js|svg|woff|woff2|ttf|eot)$ {
        expires 30d;
        add_header Cache-Control "public, immutable";
        access_log off;
    }

    # Deny hidden files
    location ~ /\. {
        deny all;
    }

    # Rate limiting para login
    location ~ ^/(login|api/auth) {
        limit_req zone=login burst=5 nodelay;
        try_files $uri $uri/ /index.php?$query_string;
    }

    # Rate limiting para API
    location /api {
        limit_req zone=api burst=30 nodelay;
        try_files $uri $uri/ /index.php?$query_string;
    }

    # Laravel routing
    location / {
        try_files $uri $uri/ /index.php?$query_string;
    }

    # PHP processing
    location ~ \.php$ {
        fastcgi_split_path_info ^(.+\.php)(/.+)$;
        fastcgi_pass php-fpm;
        fastcgi_index index.php;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
        fastcgi_param PATH_INFO $fastcgi_path_info;
        include fastcgi_params;

        fastcgi_buffering on;
        fastcgi_buffer_size 16k;
        fastcgi_buffers 16 16k;
        fastcgi_read_timeout 60s;
    }
}

# HTTPS (quando configurado)
server {
    listen 443 ssl http2;
    listen [::]:443 ssl http2;
    server_name erp.staccato.com.br;

    ssl_certificate /etc/letsencrypt/live/erp.staccato.com.br/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/erp.staccato.com.br/privkey.pem;
    ssl_session_timeout 1d;
    ssl_session_cache shared:SSL:50m;
    ssl_session_tickets off;

    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256;
    ssl_prefer_server_ciphers off;

    # HSTS
    add_header Strict-Transport-Security "max-age=63072000" always;

    # ... resto da configuração igual ao HTTP
    root /var/www/html/public;
    index index.php;

    location / {
        try_files $uri $uri/ /index.php?$query_string;
    }

    location ~ \.php$ {
        fastcgi_split_path_info ^(.+\.php)(/.+)$;
        fastcgi_pass php-fpm;
        fastcgi_index index.php;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
        include fastcgi_params;
    }
}
```

---

## Pipeline CI/CD (GitHub Actions)

### Workflow Principal

```yaml
# .github/workflows/ci-cd.yml
name: CI/CD Pipeline

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]
  release:
    types: [published]

env:
  PHP_VERSION: '8.3'
  NODE_VERSION: '20'
  REGISTRY: ghcr.io
  IMAGE_NAME: ${{ github.repository }}

jobs:
  # ==========================================
  # Testes
  # ==========================================
  test:
    name: Tests
    runs-on: ubuntu-latest

    services:
      postgres:
        image: postgres:16
        env:
          POSTGRES_USER: test
          POSTGRES_PASSWORD: test
          POSTGRES_DB: test
        ports:
          - 5432:5432
        options: >-
          --health-cmd pg_isready
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5

      redis:
        image: redis:7-alpine
        ports:
          - 6379:6379

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup PHP
        uses: shivammathur/setup-php@v2
        with:
          php-version: ${{ env.PHP_VERSION }}
          extensions: pdo_pgsql, pgsql, redis, gd, zip, intl, bcmath
          coverage: xdebug

      - name: Get Composer Cache Directory
        id: composer-cache
        run: echo "dir=$(composer config cache-files-dir)" >> $GITHUB_OUTPUT

      - name: Cache Composer dependencies
        uses: actions/cache@v4
        with:
          path: ${{ steps.composer-cache.outputs.dir }}
          key: ${{ runner.os }}-composer-${{ hashFiles('**/composer.lock') }}
          restore-keys: ${{ runner.os }}-composer-

      - name: Install Composer dependencies
        run: composer install --prefer-dist --no-progress

      - name: Setup Node.js
        uses: actions/setup-node@v4
        with:
          node-version: ${{ env.NODE_VERSION }}
          cache: 'npm'

      - name: Install NPM dependencies
        run: npm ci

      - name: Build assets
        run: npm run build

      - name: Prepare environment
        run: |
          cp .env.testing .env
          php artisan key:generate
          php artisan config:cache

      - name: Run migrations
        run: php artisan migrate --force
        env:
          DB_CONNECTION: pgsql
          DB_HOST: localhost
          DB_PORT: 5432
          DB_DATABASE: test
          DB_USERNAME: test
          DB_PASSWORD: test

      - name: Run PHPUnit tests
        run: php artisan test --coverage-clover=coverage.xml
        env:
          DB_CONNECTION: pgsql
          DB_HOST: localhost
          DB_DATABASE: test
          DB_USERNAME: test
          DB_PASSWORD: test
          REDIS_HOST: localhost

      - name: Upload coverage to Codecov
        uses: codecov/codecov-action@v4
        with:
          file: ./coverage.xml
          fail_ci_if_error: false

  # ==========================================
  # Análise Estática
  # ==========================================
  static-analysis:
    name: Static Analysis
    runs-on: ubuntu-latest

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup PHP
        uses: shivammathur/setup-php@v2
        with:
          php-version: ${{ env.PHP_VERSION }}

      - name: Install dependencies
        run: composer install --prefer-dist --no-progress

      - name: Run PHPStan
        run: vendor/bin/phpstan analyse --memory-limit=2G

      - name: Run Pint (code style)
        run: vendor/bin/pint --test

  # ==========================================
  # Testes E2E
  # ==========================================
  e2e:
    name: E2E Tests
    runs-on: ubuntu-latest
    needs: [test]

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup PHP
        uses: shivammathur/setup-php@v2
        with:
          php-version: ${{ env.PHP_VERSION }}
          extensions: pdo_pgsql

      - name: Setup Node.js
        uses: actions/setup-node@v4
        with:
          node-version: ${{ env.NODE_VERSION }}
          cache: 'npm'

      - name: Install dependencies
        run: |
          composer install --prefer-dist --no-progress
          npm ci

      - name: Build assets
        run: npm run build

      - name: Start server
        run: php artisan serve &
        env:
          APP_ENV: testing

      - name: Run Cypress
        uses: cypress-io/github-action@v6
        with:
          wait-on: 'http://localhost:8000'
          wait-on-timeout: 120

  # ==========================================
  # Build Docker Image
  # ==========================================
  build:
    name: Build Docker Image
    runs-on: ubuntu-latest
    needs: [test, static-analysis]
    if: github.event_name != 'pull_request'

    permissions:
      contents: read
      packages: write

    outputs:
      image-tag: ${{ steps.meta.outputs.tags }}
      image-digest: ${{ steps.build-push.outputs.digest }}

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3

      - name: Login to GitHub Container Registry
        uses: docker/login-action@v3
        with:
          registry: ${{ env.REGISTRY }}
          username: ${{ github.actor }}
          password: ${{ secrets.GITHUB_TOKEN }}

      - name: Extract metadata
        id: meta
        uses: docker/metadata-action@v5
        with:
          images: ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}
          tags: |
            type=ref,event=branch
            type=sha,prefix=
            type=semver,pattern={{version}}
            type=semver,pattern={{major}}.{{minor}}

      - name: Build and push
        id: build-push
        uses: docker/build-push-action@v5
        with:
          context: .
          file: docker/Dockerfile
          push: true
          tags: ${{ steps.meta.outputs.tags }}
          labels: ${{ steps.meta.outputs.labels }}
          cache-from: type=gha
          cache-to: type=gha,mode=max
          build-args: |
            APP_VERSION=${{ github.ref_name }}
            BUILD_DATE=${{ github.event.head_commit.timestamp }}
            VCS_REF=${{ github.sha }}

  # ==========================================
  # Deploy Staging
  # ==========================================
  deploy-staging:
    name: Deploy to Staging
    runs-on: ubuntu-latest
    needs: [build]
    if: github.ref == 'refs/heads/develop'
    environment:
      name: staging
      url: https://staging.erp.staccato.com.br

    steps:
      - name: Deploy to staging server
        uses: appleboy/ssh-action@v1.0.0
        with:
          host: ${{ secrets.STAGING_HOST }}
          username: ${{ secrets.STAGING_USER }}
          key: ${{ secrets.STAGING_SSH_KEY }}
          script: |
            cd /opt/staccato
            docker compose pull
            docker compose up -d --force-recreate
            docker exec staccato_app php artisan migrate --force
            docker exec staccato_app php artisan config:cache
            docker exec staccato_app php artisan route:cache
            docker exec staccato_app php artisan view:cache
            docker exec staccato_app php artisan queue:restart

  # ==========================================
  # Deploy Production
  # ==========================================
  deploy-production:
    name: Deploy to Production
    runs-on: ubuntu-latest
    needs: [build, e2e]
    if: github.event_name == 'release'
    environment:
      name: production
      url: https://erp.staccato.com.br

    steps:
      - name: Deploy to production server
        uses: appleboy/ssh-action@v1.0.0
        with:
          host: ${{ secrets.PRODUCTION_HOST }}
          username: ${{ secrets.PRODUCTION_USER }}
          key: ${{ secrets.PRODUCTION_SSH_KEY }}
          script: |
            cd /opt/staccato

            # Backup antes do deploy
            docker exec staccato_db pg_dump -U staccato staccato > /backups/pre-deploy-$(date +%Y%m%d_%H%M%S).sql

            # Pull nova imagem
            docker compose pull

            # Deploy com zero-downtime
            docker compose up -d --force-recreate --scale app=2
            sleep 30

            # Migrations
            docker exec staccato_app php artisan migrate --force

            # Cache
            docker exec staccato_app php artisan config:cache
            docker exec staccato_app php artisan route:cache
            docker exec staccato_app php artisan view:cache
            docker exec staccato_app php artisan event:cache

            # Restart queue workers
            docker exec staccato_app php artisan queue:restart

            # Scale back to 1
            docker compose up -d --scale app=1

            # Health check
            curl -f https://erp.staccato.com.br/health || exit 1
```

---

## Gerenciamento de Ambientes

### Estrutura de Ambientes

```
┌─────────────────────────────────────────────────────────────┐
│                     Ambientes                                │
├──────────────┬──────────────┬──────────────┬───────────────┤
│    Local     │   Testing    │   Staging    │  Production   │
├──────────────┼──────────────┼──────────────┼───────────────┤
│ Docker       │ GitHub       │ VPS          │ VPS/Cloud     │
│ Compose Dev  │ Actions      │ Dedicado     │ Dedicado      │
├──────────────┼──────────────┼──────────────┼───────────────┤
│ SQLite/PG    │ PostgreSQL   │ PostgreSQL   │ PostgreSQL    │
│ local        │ ephemeral    │ dedicado     │ dedicado+rep  │
├──────────────┼──────────────┼──────────────┼───────────────┤
│ Debug ON     │ Debug OFF    │ Debug OFF    │ Debug OFF     │
│ Logs verbose │ Logs minimal │ Logs normal  │ Logs minimal  │
└──────────────┴──────────────┴──────────────┴───────────────┘
```

### .env.example

```env
# ===========================================
# Aplicação
# ===========================================
APP_NAME="ERP Staccato"
APP_ENV=local
APP_KEY=
APP_DEBUG=true
APP_TIMEZONE=America/Sao_Paulo
APP_URL=http://localhost
APP_LOCALE=pt_BR

# ===========================================
# Log
# ===========================================
LOG_CHANNEL=stack
LOG_STACK=daily
LOG_DEPRECATIONS_CHANNEL=null
LOG_LEVEL=debug

# ===========================================
# Database
# ===========================================
DB_CONNECTION=pgsql
DB_HOST=127.0.0.1
DB_PORT=5432
DB_DATABASE=staccato
DB_USERNAME=staccato
DB_PASSWORD=

# ===========================================
# Cache & Session
# ===========================================
CACHE_STORE=redis
SESSION_DRIVER=redis
SESSION_LIFETIME=120

# ===========================================
# Queue
# ===========================================
QUEUE_CONNECTION=redis
QUEUE_RETRY_AFTER=90

# ===========================================
# Redis
# ===========================================
REDIS_CLIENT=phpredis
REDIS_HOST=127.0.0.1
REDIS_PORT=6379
REDIS_PASSWORD=null

# ===========================================
# Mail
# ===========================================
MAIL_MAILER=smtp
MAIL_HOST=
MAIL_PORT=587
MAIL_USERNAME=
MAIL_PASSWORD=
MAIL_ENCRYPTION=tls
MAIL_FROM_ADDRESS=
MAIL_FROM_NAME="${APP_NAME}"

# ===========================================
# Integrações
# ===========================================
# ACBr
ACBR_HOST=localhost
ACBR_PORT=3434

# Google Maps
GOOGLE_MAPS_API_KEY=

# Sentry
SENTRY_LARAVEL_DSN=
SENTRY_TRACES_SAMPLE_RATE=0.1

# ===========================================
# Storage
# ===========================================
FILESYSTEM_DISK=local

# ===========================================
# Feature Flags
# ===========================================
FEATURE_NFE_ENABLED=true
FEATURE_CNAB_ENABLED=true
```

### Variáveis por Ambiente

| Variável | Local | Staging | Production |
|----------|-------|---------|------------|
| `APP_ENV` | local | staging | production |
| `APP_DEBUG` | true | false | false |
| `LOG_LEVEL` | debug | info | warning |
| `CACHE_STORE` | array | redis | redis |
| `SESSION_DRIVER` | file | redis | redis |
| `QUEUE_CONNECTION` | sync | redis | redis |
| `MAIL_MAILER` | log | smtp | smtp |

---

## Scripts de Deploy

### entrypoint.sh

```bash
#!/bin/sh
set -e

# Aguardar banco de dados
echo "Waiting for database..."
until pg_isready -h ${DB_HOST:-db} -U ${DB_USERNAME:-staccato} -d ${DB_DATABASE:-staccato}; do
    sleep 2
done
echo "Database is ready!"

# Aguardar Redis
echo "Waiting for Redis..."
until redis-cli -h ${REDIS_HOST:-redis} ping; do
    sleep 2
done
echo "Redis is ready!"

# Criar diretórios necessários
mkdir -p /var/www/html/storage/framework/{cache,sessions,views}
mkdir -p /var/www/html/storage/logs
mkdir -p /var/www/html/bootstrap/cache

# Permissões
chown -R www-data:www-data /var/www/html/storage /var/www/html/bootstrap/cache

# Migrations (apenas se não for worker)
if [ "$1" != "queue" ] && [ "$1" != "schedule" ]; then
    echo "Running migrations..."
    php artisan migrate --force
fi

# Cache de configuração (apenas produção)
if [ "$APP_ENV" = "production" ]; then
    echo "Caching configuration..."
    php artisan config:cache
    php artisan route:cache
    php artisan view:cache
    php artisan event:cache
fi

# Executar comando
exec "$@"
```

### backup.sh

```bash
#!/bin/bash
set -e

BACKUP_DIR="/backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE="${BACKUP_DIR}/staccato_${TIMESTAMP}.sql.gz"
RETENTION_DAYS=30

echo "Starting backup at $(date)"

# Criar backup comprimido
pg_dump -Fc ${PGDATABASE} | gzip > ${BACKUP_FILE}

echo "Backup created: ${BACKUP_FILE}"
echo "Size: $(du -h ${BACKUP_FILE} | cut -f1)"

# Limpar backups antigos
find ${BACKUP_DIR} -name "staccato_*.sql.gz" -mtime +${RETENTION_DAYS} -delete

echo "Cleanup complete. Backups older than ${RETENTION_DAYS} days removed."
echo "Backup finished at $(date)"
```

### healthcheck.sh

```bash
#!/bin/sh

# Verificar PHP-FPM
if ! curl -sf http://localhost/ping > /dev/null 2>&1; then
    echo "PHP-FPM is not responding"
    exit 1
fi

# Verificar Nginx
if ! curl -sf http://localhost/health > /dev/null 2>&1; then
    echo "Nginx is not responding"
    exit 1
fi

# Verificar conexão com banco
if ! php artisan tinker --execute="DB::connection()->getPdo()" > /dev/null 2>&1; then
    echo "Database connection failed"
    exit 1
fi

# Verificar Redis
if ! php artisan tinker --execute="Redis::ping()" > /dev/null 2>&1; then
    echo "Redis connection failed"
    exit 1
fi

echo "All health checks passed"
exit 0
```

---

## Monitoramento e Alertas

### Configuração Sentry

```php
// config/sentry.php
return [
    'dsn' => env('SENTRY_LARAVEL_DSN'),
    'release' => env('APP_VERSION', '1.0.0'),
    'environment' => env('APP_ENV', 'production'),
    'traces_sample_rate' => (float) env('SENTRY_TRACES_SAMPLE_RATE', 0.1),
    'profiles_sample_rate' => 0.1,
    'send_default_pii' => false,
    'before_send' => function (\Sentry\Event $event): ?\Sentry\Event {
        // Filtrar dados sensíveis
        return $event;
    },
];
```

### Health Check Endpoint

```php
// routes/api.php
Route::get('/health', function () {
    $checks = [
        'status' => 'ok',
        'timestamp' => now()->toIso8601String(),
        'services' => [
            'database' => false,
            'redis' => false,
            'queue' => false,
        ],
    ];

    try {
        DB::connection()->getPdo();
        $checks['services']['database'] = true;
    } catch (\Exception $e) {
        $checks['status'] = 'degraded';
    }

    try {
        Redis::ping();
        $checks['services']['redis'] = true;
    } catch (\Exception $e) {
        $checks['status'] = 'degraded';
    }

    try {
        $checks['services']['queue'] = Queue::size() >= 0;
    } catch (\Exception $e) {
        $checks['status'] = 'degraded';
    }

    $statusCode = $checks['status'] === 'ok' ? 200 : 503;
    return response()->json($checks, $statusCode);
});
```

---

## Rollback e Recovery

### Procedimento de Rollback

```bash
#!/bin/bash
# rollback.sh

VERSION_ANTERIOR=$1

if [ -z "$VERSION_ANTERIOR" ]; then
    echo "Uso: ./rollback.sh <versao>"
    exit 1
fi

echo "Iniciando rollback para versão ${VERSION_ANTERIOR}..."

# 1. Parar aplicação
docker compose stop app queue scheduler

# 2. Restaurar backup do banco (se necessário)
read -p "Restaurar backup do banco? (s/N): " RESTORE_DB
if [ "$RESTORE_DB" = "s" ]; then
    BACKUP_FILE=$(ls -t /backups/pre-deploy-*.sql | head -1)
    docker exec staccato_db psql -U staccato -d staccato < $BACKUP_FILE
fi

# 3. Voltar para imagem anterior
docker compose pull --quiet
docker tag ghcr.io/staccato/erp:${VERSION_ANTERIOR} ghcr.io/staccato/erp:latest

# 4. Reiniciar serviços
docker compose up -d

# 5. Verificar health
sleep 30
curl -f https://erp.staccato.com.br/health || exit 1

echo "Rollback concluído com sucesso!"
```

### Backup e Restore

```bash
# Backup manual
docker exec staccato_db pg_dump -U staccato -Fc staccato > backup.dump

# Restore
docker exec -i staccato_db pg_restore -U staccato -d staccato --clean < backup.dump
```

---

## Checklist de Deploy

### Pré-Deploy

- [ ] Testes passando (CI)
- [ ] Code review aprovado
- [ ] Migrations testadas localmente
- [ ] Variáveis de ambiente configuradas
- [ ] Backup do banco realizado
- [ ] Equipe notificada

### Deploy

- [ ] Pull da nova imagem
- [ ] Executar migrations
- [ ] Cache de configuração
- [ ] Restart de workers
- [ ] Health check OK

### Pós-Deploy

- [ ] Verificar logs de erro
- [ ] Monitorar métricas
- [ ] Testar funcionalidades críticas
- [ ] Confirmar notificações/emails
- [ ] Documentar issues encontrados

---

## Documentos Relacionados

- [04-infraestrutura.md](./04-infraestrutura.md) - Requisitos de infraestrutura
- [07-testes.md](./07-testes.md) - Estratégia de testes (CI)
- [08-erros-monitoramento.md](./08-erros-monitoramento.md) - Monitoramento e alertas
- [05-seguranca.md](./05-seguranca.md) - Configurações de segurança
