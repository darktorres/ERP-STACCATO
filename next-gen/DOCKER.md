# Docker Setup Guide - Staccato ERP

This guide explains how to use Docker for local development and production deployment of the Staccato ERP Laravel application.

## Quick Start

### Prerequisites
- Docker >= 24.0
- Docker Compose >= 2.20
- 4GB RAM minimum (8GB recommended)
- 10GB free disk space

### Development Environment

```bash
# Clone and setup
git clone <repository>
cd erp-staccato/next-gen

# Copy environment file
cp .env.example .env

# Start development services
docker-compose -f docker-compose.dev.yml up -d

# Wait for services to be ready
sleep 10

# Install dependencies
docker-compose -f docker-compose.dev.yml exec app composer install
docker-compose -f docker-compose.dev.yml exec app npm install

# Generate application key
docker-compose -f docker-compose.dev.yml exec app php artisan key:generate

# Run migrations
docker-compose -f docker-compose.dev.yml exec app php artisan migrate

# Create admin user
docker-compose -f docker-compose.dev.yml exec app php artisan tinker
# Inside tinker shell:
# User::create(['name' => 'Admin', 'email' => 'admin@staccato.local', 'password' => Hash::make('password')])

# Access the application
# Web: http://localhost:8000
# API: http://localhost:8000/api
# Mailpit: http://localhost:8025
```

### Production Environment

```bash
# Copy environment file and configure
cp .env.example .env
# Edit .env with production settings (DB credentials, API keys, etc.)

# Start production services
docker-compose -f docker-compose.yml up -d

# Application will be available at configured APP_URL
```

## Docker Architecture

### Services

#### Development (docker-compose.dev.yml)

| Service | Image | Port | Purpose |
|---------|-------|------|---------|
| **app** | PHP 8.3 FPM Alpine | 8000 | Laravel application with Xdebug |
| **db** | PostgreSQL 16 Alpine | 5433 | Database server |
| **redis** | Redis 7 Alpine | 6380 | Cache and session storage |
| **mailpit** | Mailpit | 8025 | Email testing interface |

#### Production (docker-compose.yml)

| Service | Image | Purpose |
|---------|-------|---------|
| **app** | PHP 8.3 FPM Alpine | Laravel application |
| **queue** | PHP 8.3 FPM Alpine | Background job processing |
| **scheduler** | PHP 8.3 FPM Alpine | Task scheduling |
| **db** | PostgreSQL 16 Alpine | Database server |
| **redis** | Redis 7 Alpine | Cache and session storage |
| **backup** | PostgreSQL 16 Alpine | Automated backups |

### Volumes

#### Development
- `./:/var/www/html` - Application source code (hot reload)
- `postgres_data_dev` - PostgreSQL data persistence
- `redis_data_dev` - Redis data persistence

#### Production
- `storage_logs` - Application logs
- `storage_app` - Uploaded files and application storage
- `postgres_data` - PostgreSQL data persistence
- `redis_data` - Redis data persistence
- `ssl_certs` - SSL/TLS certificates
- `backups` - Database backups

## Configuration Files

### Environment Variables (.env)

Key variables to configure:

```bash
# Application
APP_ENV=production|local
APP_DEBUG=false|true
APP_URL=https://staccato.example.com

# Database
DB_CONNECTION=pgsql
DB_HOST=db
DB_PORT=5432
DB_DATABASE=staccato
DB_USERNAME=staccato
DB_PASSWORD=your_secure_password

# Cache & Session
CACHE_DRIVER=redis
SESSION_DRIVER=redis
REDIS_HOST=redis
REDIS_PORT=6379

# Mail
MAIL_MAILER=smtp
MAIL_HOST=smtp.mailtrap.io
MAIL_PORT=587
MAIL_USERNAME=your_username
MAIL_PASSWORD=your_password
MAIL_FROM_ADDRESS=noreply@staccato.local

# Features
FEATURE_AUDIT_TRAIL=true
FEATURE_RBAC=true
FEATURE_APPROVAL_WORKFLOW=true
FEATURE_PROGRESSIVE_DISCOUNTS=true
FEATURE_NOTIFICATIONS=true

# Docker specific
RUN_MIGRATIONS=true
RUN_SEEDERS=false
CREATE_ADMIN_USER=true
```

### PHP Configuration

**Development** (`docker/php/php-dev.ini`):
- Memory limit: 512M
- Execution time: 300s
- Error reporting: On
- OPcache validation: On (for development)
- Xdebug: Enabled

**Production** (`docker/php/php.ini`):
- Memory limit: 256M
- Execution time: 60s
- Error reporting: Off
- OPcache: Enabled with JIT compilation
- Xdebug: Disabled

### Nginx Configuration

**Main config** (`docker/nginx/nginx.conf`):
- Worker processes: auto
- Worker connections: 4096
- Gzip compression: Enabled
- Rate limiting: 60r/m API, 10r/m login
- Client max body size: 100M

**Site config** (`docker/nginx/sites/default.conf`):
- Health endpoint: `/health` (port 80)
- API endpoint: `/api`
- Static asset caching: 30 days
- Security headers: X-Frame-Options, X-Content-Type-Options, X-XSS-Protection
- FastCGI buffering: Enabled (16k buffers)

### PHP-FPM Configuration

**Pool settings** (`docker/php/www.conf`):
- Process manager: Dynamic
- Max children: 50
- Start servers: 10
- Min spare servers: 5
- Max spare servers: 20
- Max requests: 1000
- Slow log threshold: 10s
- Request timeout: 60s

## Common Commands

### Development

```bash
# Start services
docker-compose -f docker-compose.dev.yml up -d

# Stop services
docker-compose -f docker-compose.dev.yml down

# View logs
docker-compose -f docker-compose.dev.yml logs -f app
docker-compose -f docker-compose.dev.yml logs -f db

# Execute artisan commands
docker-compose -f docker-compose.dev.yml exec app php artisan tinker
docker-compose -f docker-compose.dev.yml exec app php artisan make:model YourModel -m

# Run tests
docker-compose -f docker-compose.dev.yml exec app php artisan test

# Database access
docker-compose -f docker-compose.dev.yml exec db psql -U staccato -d staccato

# Redis access
docker-compose -f docker-compose.dev.yml exec redis redis-cli

# View mails
# Visit http://localhost:8025
```

### Production

```bash
# Start services
docker-compose up -d

# Stop services gracefully
docker-compose down

# View application logs
docker-compose logs -f app

# Manual backup
docker-compose -f docker-compose.yml exec backup bash /scripts/backup.sh

# Health check
docker-compose -f docker-compose.yml exec app bash /scripts/healthcheck.sh

# Database access (be careful in production!)
docker-compose -f docker-compose.yml exec db psql -U staccato -d staccato
```

## Debugging

### Xdebug Setup (Development)

1. **VS Code Configuration** (`.vscode/launch.json`):
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Listen for XDebug",
            "type": "php",
            "port": 9003,
            "pathMapping": {
                "/var/www/html": "${workspaceFolder}/next-gen"
            }
        }
    ]
}
```

2. **Start Debugging**:
   - Set breakpoints in VS Code
   - Make request to `http://localhost:8000`
   - Execution pauses at breakpoints

### Common Issues

#### Port Already in Use
```bash
# Check what's using the port
lsof -i :8000

# Kill the process
kill -9 <PID>
```

#### Database Connection Failed
```bash
# Check PostgreSQL is running
docker-compose -f docker-compose.dev.yml ps db

# Check logs
docker-compose -f docker-compose.dev.yml logs db

# Restart database
docker-compose -f docker-compose.dev.yml restart db
```

#### Redis Connection Issues
```bash
# Check Redis is running
docker-compose -f docker-compose.dev.yml exec redis redis-cli ping
# Should return: PONG
```

#### Permission Issues
```bash
# Fix storage permissions
docker-compose -f docker-compose.dev.yml exec app chmod -R 775 storage bootstrap/cache
```

## Backup Strategy

### Automatic Backups (Production)

- **Frequency**: Daily at 2 AM UTC
- **Retention**: 30 days
- **Location**: `/backups` volume (should be mounted to persistent storage)
- **Compression**: gzip (typically 10-20% of original size)
- **Naming**: `staccato_YYYYMMDD_HHMMSS.sql.gz`

### Manual Backup

```bash
# Create backup
docker-compose -f docker-compose.yml exec backup bash /scripts/backup.sh

# Restore from backup
docker-compose -f docker-compose.yml exec db gunzip < /backups/staccato_20240110_020000.sql.gz | psql -U staccato -d staccato
```

## Production Deployment

### Checklist

- [ ] Environment variables configured (`.env`)
- [ ] SSL certificates installed in `./ssl_certs/`
- [ ] Backups mounted to persistent storage
- [ ] Database backups tested and verified
- [ ] Health checks passing
- [ ] Monitoring and alerts configured
- [ ] Firewall rules configured
- [ ] Load balancer configured (if using)

### Zero-Downtime Deployment

```bash
# 1. Pull latest code
git pull origin master

# 2. Build new image (optional if using registry)
docker-compose build --no-cache

# 3. Pull latest images
docker-compose pull

# 4. Start new containers (old ones will be gracefully shutdown)
docker-compose up -d

# 5. Verify health
curl https://your-domain.com/health

# 6. Check logs for errors
docker-compose logs -f app
```

## Monitoring and Health

### Health Checks

All services have health checks configured:

```bash
# Manual health check
docker-compose -f docker-compose.yml exec app bash /scripts/healthcheck.sh

# Docker health status
docker-compose ps
```

### Log Aggregation

```bash
# View all logs
docker-compose logs

# View specific service logs
docker-compose logs -f app
docker-compose logs -f db

# Filter by time
docker-compose logs --since 1h app

# Follow new logs
docker-compose logs -f
```

## Security Considerations

1. **Environment Variables**: Never commit `.env` with production secrets
2. **SSL/TLS**: Use proper certificates in production
3. **Database**: Use strong passwords, restrict network access
4. **Backups**: Store backups in secure, separate location
5. **Updates**: Regularly update base images and dependencies
6. **Network**: Use bridge networks, avoid exposing internal services
7. **Permissions**: Run services as non-root users

## Performance Tuning

### For Large Datasets

```bash
# Increase PHP memory limit
# Edit docker/php/php.ini
memory_limit=512M

# Increase PostgreSQL settings
# docker-compose.yml - add to postgres service:
environment:
  POSTGRES_INITDB_ARGS: "-c max_connections=200 -c shared_buffers=256MB"

# Increase Redis memory
# docker-compose.yml - add to redis command
command: redis-server --maxmemory 256mb --maxmemory-policy allkeys-lru
```

### Database Indexing

After initial deployment, create indexes for frequently queried fields:

```bash
docker-compose exec db psql -U staccato -d staccato << EOF
CREATE INDEX idx_usuarios_email ON usuarios(email);
CREATE INDEX idx_produtos_sku ON produtos(sku);
CREATE INDEX idx_vendas_cliente_id ON vendas(cliente_id);
-- Add more indexes as needed based on query analysis
EOF
```

## References

- [Docker Official Docs](https://docs.docker.com/)
- [Docker Compose Reference](https://docs.docker.com/compose/compose-file/)
- [Laravel Docker Documentation](https://laravel.com/docs/deployment)
- [PostgreSQL in Docker](https://hub.docker.com/_/postgres)
- [PHP-FPM Configuration](https://www.php.net/manual/en/install.fpm.configuration.php)
- [Nginx Configuration](https://nginx.org/en/docs/)

## Support

For issues or questions:
1. Check Docker logs: `docker-compose logs`
2. Review `.env` configuration
3. Verify all services are running: `docker-compose ps`
4. Check GitHub issues: [Repository Issues](https://github.com/your-repo/issues)
