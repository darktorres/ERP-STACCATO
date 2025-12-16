# Playwright E2E Tests for Staccato ERP

Comprehensive end-to-end test suite for the Staccato ERP web application using Playwright.

## Test Coverage

### 1. Authentication Tests (`auth.spec.js`)
- **Login Form Structure**: Verifies all form elements are present and visible
- **Form Validation**: Tests empty input handling and validation messages
- **Invalid Credentials**: Tests error messages for wrong login attempts
- **Remember Username**: Tests checkbox functionality and cookie storage
- **Successful Login**: Tests login with valid credentials (requires database setup)
- **Form Styling**: Verifies Bootstrap classes and CSS styling
- **Page Structure**: Tests layout and component visibility

**Tests:**
- `should display login form with correct elements`
- `should show error for empty credentials`
- `should show error for invalid credentials`
- `should remember username when checkbox is checked`
- `should redirect to orcamentos on successful login`
- `should display loading state on form submission`
- `should have proper styling and layout`
- `should show footer with version info`

### 2. Quotations Page Tests (`quotations.spec.js`)
- **Page Structure**: Tests sidebar, main content, navbar layout
- **Filter Elements**: Verifies all filter inputs and options
- **Status Checkboxes**: Tests all status filter options
- **Semáforo Filters**: Tests traffic light filtering
- **AJAX Functionality**: Tests dynamic data loading and updates
- **Table Display**: Tests quotation table rendering and formatting
- **Dropdown Population**: Tests dynamic dropdown loading
- **Responsive Design**: Tests mobile and tablet viewports
- **User Session**: Tests user info display and logout

**Filter Tests:**
- Store filtering
- Month/Period filtering
- Vendor/Seller filtering
- Supplier filtering
- Status filtering (ATIVO, CANCELADO, EXPIRADO, FECHADO, PERDIDO, REPLICADO)
- Semáforo filtering (Quente, Morno, Frio)
- Own quotes filtering (for sellers)
- Text search

**Display Tests:**
- Table column rendering
- Status badge color coding
- Date formatting (DD/MM/YYYY Portuguese format)
- Currency formatting (R$ format)
- Semáforo emoji indicators

### 3. API Tests (`api.spec.js`)
- **Endpoint Availability**: Tests all AJAX endpoints respond correctly
- **Response Format**: Validates JSON response structure
- **Data Structure**: Tests required fields in responses
- **Navigation**: Tests page accessibility
- **Form Handling**: Tests form attributes and validation
- **Security**: Tests for sensitive information leaks
- **Database Connection**: Tests database connectivity

**Endpoints Tested:**
- `GET /orcamentos/lojas` - Store dropdown data
- `GET /orcamentos/vendedores` - Seller dropdown data
- `GET /orcamentos/fornecedores` - Supplier dropdown data
- `GET /orcamentos/data` - Quotation list with filtering
- `GET /` - Login page
- `GET /orcamentos` - Quotations page
- `GET /logout` - Logout endpoint

## Running Tests

### Run All Tests
```bash
npm test
```

### Run Specific Test Suite
```bash
npm run test:auth          # Authentication tests only
npm run test:quotations    # Quotations page tests only
npm run test:api           # API endpoint tests only
```

### Run Tests in Debug Mode
```bash
npm run test:debug
```

### Run Tests with Browser Visible
```bash
npm run test:headed
```

### View Test Report
```bash
npm run test:report
```

## Prerequisites

1. **MySQL Database**: `staccato` database must exist with schema
2. **PHP Server**: Web server must be running on `http://localhost:8000`
3. **Node.js**: 14+ installed
4. **Playwright Browsers**: Automatically installed during `npm install`

### Database Setup for Full Testing

For tests that require valid credentials:

1. Create a test user in the `usuario` table:
   ```sql
   INSERT INTO usuario (user, password, nome, tipo, idLoja, desativado)
   VALUES ('testuser', SHA_PASSWORD('senha'), 'Test User', 'GERENTE LOJA', 1, FALSE);
   ```

2. Update test files to use valid credentials:
   - Modify `auth.spec.js` login tests
   - Update `quotations.spec.js` AJAX tests

## Test Structure

### Fixtures (`fixtures.js`)
- `clearSession`: Clears cookies before each test
- `login`: Helper function to log in as a user

### Configuration (`playwright.config.js`)
- Base URL: `http://localhost:8000`
- Browser: Chromium
- Screenshot on failure
- Trace on first retry
- Auto-start PHP dev server

## Test Categories

### Skipped Tests
Some tests are marked with `test.skip()` because they require:
- Valid database credentials
- Logged-in user session
- Populated database with quotation data

To enable these tests:
1. Set up test database with sample data
2. Remove `test.skip()` calls
3. Update credentials in test files
4. Run tests

### Structure Tests (No Login Required)
Tests that verify page structure, layout, form validation, and API endpoints work without authentication:
- Form element visibility
- Page structure
- Filter UI elements
- API endpoint availability
- Response formats

### Integration Tests (Requires Login)
Tests that verify full user workflows:
- Login flow
- Filter application
- AJAX data loading
- Table updates
- Logout flow

## Performance Considerations

- Tests run sequentially (fullyParallel: false) to avoid race conditions
- 3 workers for parallel execution when appropriate
- 120 second timeout for dev server startup
- 10 second timeout for AJAX requests

## Debugging Failed Tests

1. **Check Playwright Report**:
   ```bash
   npm run test:report
   ```

2. **Run in Debug Mode**:
   ```bash
   npm run test:debug
   ```

3. **Check Screenshots**: Generated in `test-results/` on failure

4. **Check Trace**: Trace files record full browser actions

5. **View in Headed Mode**:
   ```bash
   npm run test:headed
   ```

## Common Issues

### Database Connection
- Error: "System in maintenance!"
- Solution: Check `maintenance` table, set `emManutencao = FALSE`

### Invalid Credentials
- Error: "Login inválido!"
- Solution: Verify user exists in `usuario` table with correct password (SHA_PASSWORD)

### AJAX Timeout
- Error: Request timeout on `/orcamentos/data`
- Solution: Check PHP server is running, database is accessible

### Page Not Found
- Error: 404 on `/orcamentos`
- Solution: Ensure Symfony routes are properly configured, check `bin/console debug:router`

## Test Results

Test results are generated in:
- `test-results/` - Screenshot and trace artifacts
- `playwright-report/` - HTML test report

View report:
```bash
npm run test:report
```

## Continuous Integration

For CI/CD pipelines, set environment variable:
```bash
CI=true npm test
```

This will:
- Retry failed tests 2 times
- Use single worker (serial execution)
- Generate full artifacts

## Coverage

### Current Coverage:
- ✅ Login page structure and validation (8 tests)
- ✅ Quotations page layout and filters (22 tests)
- ✅ API endpoints and response formats (15 tests)
- ✅ Form handling and security (5 tests)
- ✅ Database connection validation (2 tests)

**Total: 52 tests**

## Future Test Additions

1. **E2E Workflows**:
   - Complete login → filter → view quotations → logout flow
   - Filter combinations and complex searches
   - Session persistence across page reloads

2. **Performance Tests**:
   - Page load time assertions
   - AJAX response time checks
   - Table rendering performance

3. **Accessibility Tests**:
   - Keyboard navigation
   - Screen reader compatibility
   - Color contrast validation

4. **Cross-browser Testing**:
   - Firefox
   - Safari
   - Edge

5. **Visual Regression Tests**:
   - Screenshot comparison
   - Layout consistency
   - Style consistency

## Contributing

When adding new tests:
1. Follow existing test structure
2. Use descriptive test names
3. Group related tests in `describe` blocks
4. Add comments for complex test logic
5. Mark data-dependent tests with `test.skip()`
6. Update this README with new test descriptions

## Resources

- [Playwright Documentation](https://playwright.dev)
- [Playwright API](https://playwright.dev/docs/api/class-playwright)
- [Best Practices](https://playwright.dev/docs/best-practices)
- [Debugging Guide](https://playwright.dev/docs/debug)
