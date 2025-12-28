# Quick Start Guide - Staccato ERP Web Application

## 30-Second Setup

```bash
# 1. Navigate to project
cd web-symfony

# 2. Install dependencies
composer install --ignore-platform-reqs
npm install

# 3. Configure database (edit .env)
# Change: mysql://loginUser:password@localhost:3306/staccato

# 4. Start server
php bin/console server:run

# 5. Open browser
# Visit: http://localhost:8000
```

## Running Tests

```bash
# All tests
npm test

# Specific test suite
npm run test:auth
npm run test:quotations
npm run test:api

# View results
npm run test:report
```

## Default Routes

| URL                                | Purpose                     |
| ---------------------------------- | --------------------------- |
| `http://localhost:8000/`           | Login page                  |
| `http://localhost:8000/orcamentos` | Quotations (requires login) |
| `http://localhost:8000/logout`     | Logout                      |

## Test Credentials

For testing (requires database setup):

```text
Username: testuser
Password: senha
```

Create test user:

```sql
INSERT INTO usuario (user, password, nome, tipo, idLoja, desativado)
VALUES ('testuser', SHA_PASSWORD('senha'), 'Test User', 'GERENTE LOJA', 1, FALSE);
```

## Features

### Login Page

- Username and password fields
- Remember username option
- Error message display
- Form validation

### Quotations Page

- Filtered list table
- 8 different filters
- AJAX dynamic updates
- Responsive design
- User session info

## Common Commands

```bash
# Check routes
php bin/console debug:router

# Clear cache
php bin/console cache:clear

# Run tests with browser visible
npm run test:headed

# Debug tests
npm run test:debug

# View test report
npm run test:report
```

## Troubleshooting

### Port 8000 in use

```bash
php bin/console server:run --port=8001
```

### Database not found

```bash
# Create database
mysql -u root -p
CREATE DATABASE staccato;
```

### Tests failing

```bash
# Run in debug mode
npm run test:debug

# Run with visible browser
npm run test:headed
```

## File Locations

- **Login Page**: `templates/auth/login.html.twig`
- **Quotations Page**: `templates/quotation/list.html.twig`
- **Controllers**: `src/Controller/`
- **Tests**: `tests/e2e/`
- **Config**: `config/`

## Environment Variables

```env
APP_ENV=dev
APP_SECRET=StaccatoERP2024!@#SymfonyWebAPI
DATABASE_URL=mysql://loginUser:password@localhost:3306/staccato?serverVersion=5.7&charset=utf8mb4
JWT_SECRET=StaccatoERP2024!@#SymfonyWebAPISecretKey
```

## Documentation

- Full implementation: `.claude/FINAL_IMPLEMENTATION_SUMMARY.md`
- Test guide: `tests/e2e/README.md`
- Test summary: `TEST_SUITE_SUMMARY.md`

## Success Checklist

- [ ] Dependencies installed (Composer + NPM)
- [ ] .env configured with database credentials
- [ ] Server running on <http://localhost:8000>
- [ ] Login page loads and displays
- [ ] All 52 tests passing
- [ ] Can login with valid credentials
- [ ] Quotations page displays and filters work

---

**Status**: ✅ Ready to Use
**Total Tests**: 52
**Pages**: 2 (Login + Quotations)
