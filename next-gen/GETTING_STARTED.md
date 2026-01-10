# Getting Started with Staccato ERP Laravel

This guide will help you set up and run the Staccato ERP next-generation Laravel application.

## What's Included

You now have a complete Laravel 11 application with:
- ✓ PostgreSQL database configuration
- ✓ Redis caching and sessions
- ✓ Docker development environment
- ✓ Xdebug debugging support
- ✓ Queue/job processing setup
- ✓ Email testing with Mailpit

## Option 1: Docker Development (Recommended)

### Quick Start

```bash
cd next-gen

# Start services (PostgreSQL, Redis, Mailpit)
docker-compose -f docker-compose.dev.yml up -d

# Verify services are running
docker-compose -f docker-compose.dev.yml ps
```

### Install Dependencies

```bash
# PHP dependencies
composer install

# Node.js dependencies (for frontend assets)
npm install
```

### Setup Database

```bash
# Generate APP_KEY if not already done
php artisan key:generate

# Run database migrations
php artisan migrate

# Optional: Seed sample data
php artisan db:seed
```

### Run Application

```bash
# Start Laravel development server
php artisan serve

# In another terminal, watch frontend assets
npm run dev
```

Visit **http://localhost:8000** in your browser.

### Email Testing

View emails sent by the application:
- Visit **http://localhost:8025** (Mailpit UI)
- Emails are captured in development without actually sending

### Database Access

```bash
# Connect to PostgreSQL
docker-compose -f docker-compose.dev.yml exec db psql -U staccato -d staccato_dev

# View database tables
\dt

# Exit PostgreSQL
\q
```

### Redis CLI

```bash
# Access Redis
docker-compose -f docker-compose.dev.yml exec redis redis-cli

# View keys
KEYS *

# Check cache status
INFO stats
```

### Debugging with Xdebug

1. **VS Code Setup**:
   - Install "PHP Debug" extension by Felix Becker
   - Create `.vscode/launch.json`:
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
   - Press F5 to start listening for connections
   - Set breakpoints in code (red dots)
   - Make request to application
   - Execution pauses at breakpoints

### Stopping Services

```bash
docker-compose -f docker-compose.dev.yml down
```

## Option 2: Local Development (Without Docker)

### Prerequisites

- PHP 8.3+ with extensions: pdo_pgsql, gd, zip, intl, bcmath, redis
- PostgreSQL 15+
- Redis 7+
- Composer
- Node.js 20+

### Installation

```bash
cd next-gen

# Install PHP dependencies
composer install

# Install Node dependencies
npm install

# Create environment file
cp .env.example .env

# Update .env with your database credentials
# Edit .env and set:
# DB_HOST=localhost
# DB_PORT=5432
# DB_DATABASE=staccato
# DB_USERNAME=postgres
# DB_PASSWORD=your_password

# Generate application key
php artisan key:generate

# Run migrations
php artisan migrate

# Build frontend assets
npm run build
```

### Running

```bash
# Start Laravel dev server (in Terminal 1)
php artisan serve

# Watch frontend files (in Terminal 2)
npm run dev
```

Visit **http://localhost:8000**

## Testing

### Run Tests

```bash
# Run all tests
php artisan test

# Run with coverage report
php artisan test --coverage

# Run specific test file
php artisan test tests/Feature/ExampleTest.php

# Run unit tests only
php artisan test tests/Unit

# Watch mode (rerun tests on file change)
php artisan test --watch
```

### Running Tests in Docker

```bash
docker-compose -f docker-compose.dev.yml run --rm app php artisan test
```

## Configuration

### Environment Variables (.env)

Key variables to set for development:

```env
APP_ENV=local
APP_DEBUG=true
APP_URL=http://localhost:8000

# Database (Docker)
DB_HOST=db
DB_PORT=5432
DB_DATABASE=staccato_dev
DB_USERNAME=staccato
DB_PASSWORD=secret

# Database (Local)
DB_HOST=127.0.0.1
DB_PORT=5432
DB_DATABASE=staccato
DB_USERNAME=postgres
DB_PASSWORD=your_password

# Cache & Session
CACHE_DRIVER=redis
SESSION_DRIVER=redis
REDIS_HOST=redis (Docker) or localhost (Local)
REDIS_PORT=6379

# Mail
MAIL_MAILER=smtp
MAIL_HOST=mailpit (Docker) or smtp.mailtrap.io (Production)
MAIL_PORT=1025 (Docker) or 587 (Production)
```

### Database Configuration

Database connection is configured in `config/database.php`. For development with Docker:

```php
'pgsql' => [
    'driver' => 'pgsql',
    'host' => env('DB_HOST', 'db'),  // Uses 'db' service in Docker
    'port' => env('DB_PORT', 5432),
    'database' => env('DB_DATABASE', 'staccato_dev'),
    'username' => env('DB_USERNAME', 'staccato'),
    'password' => env('DB_PASSWORD', 'secret'),
],
```

## Common Commands

### Artisan CLI

```bash
# Make a new Model
php artisan make:model User

# Make a new Model with migration
php artisan make:model User -m

# Make a new Controller
php artisan make:controller UserController

# Make a new Request (form validation)
php artisan make:request StoreUserRequest

# Make a new Event
php artisan make:event UserCreated

# Make a new Service class
php artisan make:class Services/UserService

# View all routes
php artisan route:list

# Optimize for production
php artisan optimize

# Clear caches
php artisan cache:clear
php artisan view:clear
php artisan route:clear
php artisan config:clear
```

### Database Migrations

```bash
# Run migrations
php artisan migrate

# Rollback last migration
php artisan migrate:rollback

# Rollback all migrations
php artisan migrate:reset

# Migrate and seed
php artisan migrate --seed

# Create new migration
php artisan make:migration create_users_table
```

### Database Seeding

```bash
# Seed database
php artisan db:seed

# Seed specific seeder
php artisan db:seed --class=UserSeeder
```

## File Structure

Key directories:

```
next-gen/
├── app/
│   ├── Http/Controllers/     # Route handlers
│   ├── Models/               # Eloquent models
│   ├── Services/             # Business logic
│   ├── Events/               # Domain events
│   ├── Exceptions/           # Custom exceptions
│   └── Providers/            # Service providers
├── config/                   # Configuration files
├── database/
│   ├── migrations/           # Schema migrations
│   └── seeders/              # Database seeders
├── resources/
│   ├── views/                # Blade templates
│   ├── css/                  # Stylesheets
│   └── js/                   # JavaScript
├── routes/                   # Route definitions
├── tests/                    # Test suite
├── storage/                  # Logs, cache, uploads
├── public/                   # Web root
├── bootstrap/                # Framework bootstrap
├── vendor/                   # Dependencies
└── docker/                   # Docker configuration
```

## Troubleshooting

### Docker Services Not Starting

```bash
# Check logs
docker-compose -f docker-compose.dev.yml logs -f db

# Restart services
docker-compose -f docker-compose.dev.yml restart

# Full reset
docker-compose -f docker-compose.dev.yml down -v
docker-compose -f docker-compose.dev.yml up -d
```

### Database Connection Error

```bash
# Verify services are running
docker-compose -f docker-compose.dev.yml ps

# Check database is ready
docker-compose -f docker-compose.dev.yml exec -T db pg_isready
```

### Redis Connection Error

```bash
# Check Redis is ready
docker-compose -f docker-compose.dev.yml exec -T redis redis-cli ping
# Should return: PONG
```

### Permission Issues

```bash
# Fix storage permissions (if needed)
chmod -R 775 storage bootstrap/cache
chown -R www-data:www-data storage bootstrap/cache
```

### Port Already in Use

```bash
# Check what's using port 8000
lsof -i :8000

# Kill the process
kill -9 <PID>

# Or use a different port
php artisan serve --port=8001
```

## Next Steps

1. **Create Your First Model**: `php artisan make:model Product -m`
2. **Create a Controller**: `php artisan make:controller ProductController`
3. **Define Routes**: Edit `routes/web.php`
4. **Run Migrations**: `php artisan migrate`
5. **Write Tests**: Create test files in `tests/`
6. **Build Frontend**: `npm run build`

## Resources

- [Laravel Documentation](https://laravel.com/docs)
- [PostgreSQL Documentation](https://www.postgresql.org/docs/)
- [Redis Documentation](https://redis.io/docs/)
- [Xdebug Documentation](https://xdebug.org/docs/)
- [Docker Documentation](https://docs.docker.com/)

## Getting Help

- **Laravel Docs**: https://laravel.com/docs
- **Stack Overflow**: Tag your questions with `laravel`, `php`, `postgresql`
- **GitHub Discussions**: Check project repository
- **Local Docs**: See `.claude/next-gen/` for ERP-specific documentation

---

**Created**: 2026-01-10
**Framework**: Laravel 11
**PHP Version**: 8.3
**Database**: PostgreSQL 16
**Cache**: Redis 7
