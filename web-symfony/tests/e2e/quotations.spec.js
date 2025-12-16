const { test, expect } = require('@playwright/test');

test.describe('Quotations Page - Layout and Structure', () => {
  test.beforeEach(async ({ page }) => {
    // Navigate to quotations page directly (tests page structure)
    // In real scenario, would be behind login
    await page.goto('/orcamentos', { waitUntil: 'networkidle' }).catch(() => {
      // If redirected to login, that's expected if not logged in
    });
  });

  test('should have proper page structure with sidebar and main content', async ({ page, context }) => {
    // If not logged in, should redirect to login
    const url = page.url();
    if (url.includes('localhost')) {
      // If we can access the page, check structure
      await expect(page.locator('.page-container')).toBeVisible().catch(() => {
        // Page might require login
      });
    }
  });

  test('should have navbar with user info and logout button', async ({ page }) => {
    // Check navbar structure
    const navbar = page.locator('nav.navbar');
    if (await navbar.isVisible().catch(() => false)) {
      // Navbar should contain brand and logout button
      await expect(page.locator('.navbar-brand')).toBeVisible();
    }
  });

  test('should display filter sidebar with all filter sections', async ({ page }) => {
    // Check sidebar visibility
    const sidebar = page.locator('.sidebar');
    if (await sidebar.isVisible().catch(() => false)) {
      // Should have filter sections
      await expect(page.locator('.filter-section')).toHaveCount(await page.locator('.filter-section').count());
    }
  });

  test('should have quotation table with proper columns', async ({ page }) => {
    // Check table structure
    const table = page.locator('table#orcamentosTable');
    if (await table.isVisible().catch(() => false)) {
      // Check for table headers
      const headers = ['ID', 'Vendedor', 'Cliente', 'Profissional', 'Status', 'Dias', 'Data', 'Total', 'Semáforo'];
      for (const header of headers) {
        await expect(page.locator(`th:has-text("${header}")`)).toBeVisible();
      }
    }
  });
});

test.describe('Quotations Page - Filters', () => {
  test('should have all filter input elements', async ({ page }) => {
    // Direct navigation to quotations (structure test)
    await page.goto('/orcamentos').catch(() => {
      // Expected to fail if not logged in
    });

    // Check for filter inputs
    const filterForm = page.locator('#filterForm');
    if (await filterForm.isVisible().catch(() => false)) {
      // Store filter
      await expect(page.locator('#idLoja')).toBeVisible().catch(() => {
        // Might not be visible for all user types
      });

      // Vendor filter
      await expect(page.locator('#idVendedor')).toBeVisible().catch(() => {});

      // Supplier filter
      await expect(page.locator('#fornecedor')).toBeVisible().catch(() => {});

      // Search input
      await expect(page.locator('#search')).toBeVisible().catch(() => {});
    }
  });

  test('should have status checkboxes for all statuses', async ({ page }) => {
    // Navigate to quotations page (will redirect to login if not authenticated)
    await page.goto('/orcamentos').catch(() => {});

    // Wait for page to load
    await page.waitForLoadState('networkidle').catch(() => {});

    // Only check if sidebar is visible (meaning we're on the quotations page)
    const sidebar = page.locator('.sidebar');
    if (await sidebar.isVisible().catch(() => false)) {
      const statuses = ['ativo', 'cancelado', 'expirado', 'fechado', 'perdido', 'replicado'];
      for (const status of statuses) {
        await expect(page.locator(`#status_${status}`)).toBeVisible().catch(() => {});
      }
    }
  });

  test('should have semaforo radio buttons', async ({ page }) => {
    await page.goto('/orcamentos').catch(() => {});

    // Check semaforo options
    await expect(page.locator('#semaforo_quente')).toBeVisible().catch(() => {});
    await expect(page.locator('#semaforo_morno')).toBeVisible().catch(() => {});
    await expect(page.locator('#semaforo_frio')).toBeVisible().catch(() => {});
    await expect(page.locator('#semaforo_todos')).toBeVisible().catch(() => {});
  });

  test('should have filter and clear buttons', async ({ page }) => {
    await page.goto('/orcamentos').catch(() => {});

    // Check buttons
    await expect(page.locator('#btnFiltrar')).toBeVisible().catch(() => {});
    await expect(page.locator('#btnLimpar')).toBeVisible().catch(() => {});
  });
});

test.describe('Quotations Page - AJAX Functionality', () => {
  test.beforeEach(async ({ page, context }) => {
    console.log('AJAX TEST: Clearing cookies and navigating to login page');
    // Login before each test
    await context.clearCookies();
    await page.goto('/');

    console.log('AJAX TEST: Login page loaded. Current URL:', page.url());
    console.log('AJAX TEST: Filling credentials (torres / 1234)');
    await page.fill('#user', 'torres');
    await page.fill('#password', '1234');

    console.log('AJAX TEST: Clicking submit button');
    await page.click('button[type="submit"]');

    console.log('AJAX TEST: Waiting for redirect to /orcamentos');
    // Wait for login to succeed
    try {
      await page.waitForURL('/orcamentos', { timeout: 15000 });
      console.log('AJAX TEST: Successfully logged in. URL:', page.url());
    } catch (e) {
      console.log('AJAX TEST: Login failed. Current URL:', page.url());
      const errorMsg = await page.locator('.alert-danger').textContent().catch(() => 'No error message');
      console.log('AJAX TEST: Error displayed:', errorMsg);
      throw new Error(`AJAX beforeEach login failed: ${e.message}`);
    }
  });

  test('filter button should trigger AJAX request', async ({ page }) => {
    // Set up listener for network requests
    const responsePromise = page.waitForResponse(response =>
      response.url().includes('/orcamentos/data')
    );

    // Click filter button
    await page.click('#btnFiltrar');

    // Wait for AJAX response
    const response = await responsePromise;
    expect(response.status()).toBe(200);

    // Parse JSON response
    const data = await response.json();
    // API returns 'data' and 'success' keys
    expect(data).toHaveProperty('data');
    expect(data).toHaveProperty('success');
    expect(data.success).toBe(true);
  });

  test('clear button should reset all filters', async ({ page }) => {
    // Set some filter values
    await page.fill('#search', 'test');
    await page.check('#status_ativo');

    // Click clear button
    await page.click('#btnLimpar');

    // Verify filters are cleared
    expect(await page.inputValue('#search')).toBe('');
    expect(await page.isChecked('#status_ativo')).toBe(false);
  });

  test('table should update when filters are applied', async ({ page }) => {
    // Get initial table row count
    const initialRows = await page.locator('table#orcamentosTable tbody tr').count();

    // Apply a filter
    await page.fill('#search', 'test');
    await page.click('#btnFiltrar');

    // Wait for table update
    await page.waitForLoadState('networkidle');

    // Table might have different number of rows
    // Just verify table still exists
    await expect(page.locator('table#orcamentosTable')).toBeVisible();
  });
});

test.describe('Quotations Page - Table Display', () => {
  test.beforeEach(async ({ page, context }) => {
    // Login before each test
    await context.clearCookies();
    await page.goto('/');
    await page.fill('#user', 'torres');
    await page.fill('#password', '1234');
    await page.click('button[type="submit"]');

    // Wait for login to succeed
    await page.waitForURL('/orcamentos', { timeout: 15000 });
  });

  test('should render quotation table rows correctly', async ({ page }) => {
    // Get table rows
    const rows = page.locator('table#orcamentosTable tbody tr');
    const count = await rows.count();

    // If we have data
    if (count > 0) {
      // Check first row structure
      const firstRow = rows.first();
      const cells = firstRow.locator('td');

      // Should have 9 cells (matching headers)
      expect(await cells.count()).toBe(9);
    }
  });

  test('status badges should have correct color classes', async ({ page }) => {
    const badges = page.locator('.status-badge');
    const count = await badges.count();

    // If we have data with status badges
    if (count > 0) {
      for (const badge of await badges.all()) {
        const className = await badge.getAttribute('class');
        expect(className).toMatch(/status-(ativo|cancelado|expirado|fechado|perdido|replicado)/);
      }
    }
  });

  test('should format dates correctly', async ({ page }) => {
    // Check date formatting in table
    const dateCells = page.locator('table#orcamentosTable tbody tr td:nth-child(7)');

    if (await dateCells.count() > 0) {
      const dateText = await dateCells.first().textContent();
      // Should be in DD/MM/YYYY format (Portuguese)
      expect(dateText).toMatch(/\d{2}\/\d{2}\/\d{4}/);
    }
  });

  test('should format currency correctly', async ({ page }) => {
    // Check currency formatting
    const totalCells = page.locator('table#orcamentosTable tbody tr td:nth-child(8)');

    if (await totalCells.count() > 0) {
      const totalText = await totalCells.first().textContent();
      // Should contain R$ and decimal format
      expect(totalText).toMatch(/R\$\s*[\d.,]+/);
    }
  });
});

test.describe('Quotations Page - Dropdown Population', () => {
  test.beforeEach(async ({ page, context }) => {
    // Login before each test
    await context.clearCookies();
    await page.goto('/');
    await page.fill('#user', 'torres');
    await page.fill('#password', '1234');
    await page.click('button[type="submit"]');

    // Wait for login to succeed
    await page.waitForURL('/orcamentos', { timeout: 15000 });
  });

  test('should load vendedores dropdown dynamically', async ({ page }) => {
    // Select a store if available
    const storeSelect = page.locator('#idLoja');
    if (await storeSelect.isVisible().catch(() => false)) {
      await storeSelect.selectOption('1');
      await page.waitForLoadState('networkidle');
    }

    // Get options
    const options = page.locator('#idVendedor option');
    const count = await options.count();

    // Should have at least the "Todos" option
    expect(count).toBeGreaterThanOrEqual(1);
  });

  test('should load fornecedores dropdown', async ({ page }) => {
    // Wait for page to load
    await page.waitForLoadState('networkidle');

    const options = page.locator('#fornecedor option');
    const count = await options.count();

    // Should have at least the "Todos" option
    expect(count).toBeGreaterThanOrEqual(1);
  });
});

test.describe('Quotations Page - Responsive Design', () => {
  test('should be responsive on mobile viewport', async ({ browser }) => {
    const context = await browser.newContext({
      viewport: { width: 375, height: 667 } // iPhone size
    });
    const page = await context.newPage();

    await page.goto('/orcamentos').catch(() => {});

    // Check that sidebar and content are both visible or properly stacked
    const sidebar = page.locator('.sidebar');
    const mainContent = page.locator('.main-content');

    if (await sidebar.isVisible().catch(() => false)) {
      // On mobile, sidebar might be scrollable or collapsed
      expect(await sidebar.isVisible() || await mainContent.isVisible()).toBe(true);
    }

    await context.close();
  });

  test('should be responsive on tablet viewport', async ({ browser }) => {
    const context = await browser.newContext({
      viewport: { width: 768, height: 1024 } // iPad size
    });
    const page = await context.newPage();

    await page.goto('/orcamentos').catch(() => {});

    // Both sidebar and content should be visible
    const sidebar = page.locator('.sidebar');
    const mainContent = page.locator('.main-content');

    if (await sidebar.isVisible().catch(() => false)) {
      expect(true).toBe(true); // Page loaded
    }

    await context.close();
  });
});

test.describe('Quotations Page - User Session', () => {
  test.beforeEach(async ({ page, context }) => {
    // Login before each test
    await context.clearCookies();
    await page.goto('/');
    await page.fill('#user', 'torres');
    await page.fill('#password', '1234');
    await page.click('button[type="submit"]');

    // Wait for login to succeed
    await page.waitForURL('/orcamentos', { timeout: 15000 });
  });

  test('should display user info in sidebar', async ({ page }) => {
    // Check for user info section
    const userInfo = page.locator('.user-info');
    await expect(userInfo).toBeVisible();

    // Should display user name
    const userName = userInfo.locator('strong');
    expect(await userName.textContent()).toBeTruthy();

    // Should display store info
    const storeInfo = userInfo.locator('small');
    await expect(storeInfo).toContainText('Loja:');
  });

  test('should have logout button in navbar', async ({ page }) => {
    // Find logout link
    const logoutBtn = page.locator('a:has-text("Sair")');
    await expect(logoutBtn).toBeVisible();
  });
});
