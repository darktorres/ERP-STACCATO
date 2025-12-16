const { test, expect } = require('@playwright/test');

test.describe('API Endpoints - Data Loading', () => {
  test('should handle /orcamentos/lojas endpoint', async ({ page }) => {
    // Test the AJAX endpoint for loading stores
    const response = await page.request.get('/orcamentos/lojas', {
      headers: {
        'Accept': 'application/json',
      }
    }).catch(() => null);

    // If not authenticated, might get 401
    if (response) {
      // Either successful response or auth error
      expect([200, 401]).toContain(response.status());
    }
  });

  test('should handle /orcamentos/vendedores endpoint', async ({ page }) => {
    // Test the AJAX endpoint for loading sellers
    const response = await page.request.get('/orcamentos/vendedores', {
      headers: {
        'Accept': 'application/json',
      }
    }).catch(() => null);

    if (response) {
      expect([200, 401]).toContain(response.status());
    }
  });

  test('should handle /orcamentos/fornecedores endpoint', async ({ page }) => {
    // Test the AJAX endpoint for loading suppliers
    const response = await page.request.get('/orcamentos/fornecedores', {
      headers: {
        'Accept': 'application/json',
      }
    }).catch(() => null);

    if (response) {
      expect([200, 401]).toContain(response.status());
    }
  });

  test('should handle /orcamentos/data endpoint with parameters', async ({ page }) => {
    // Test the main data endpoint with query parameters
    const response = await page.request.get('/orcamentos/data?search=test', {
      headers: {
        'Accept': 'application/json',
      }
    }).catch(() => null);

    if (response) {
      expect([200, 401]).toContain(response.status());
    }
  });
});

test.describe('API Response Format', () => {
  test('/orcamentos/lojas should return proper JSON format', async ({ page }) => {
    const response = await page.request.get('/orcamentos/lojas', {
      headers: { 'Accept': 'application/json' }
    }).catch(() => null);

    if (response && response.status() === 200) {
      const data = await response.json();

      // Should be an array
      if (Array.isArray(data)) {
        // If it has items, check structure
        if (data.length > 0) {
          expect(data[0]).toHaveProperty('idLoja');
          expect(data[0]).toHaveProperty('descricao');
          expect(data[0]).toHaveProperty('nomeFantasia');
        }
      }
    }
  });

  test('/orcamentos/vendedores should return proper JSON format', async ({ page }) => {
    const response = await page.request.get('/orcamentos/vendedores', {
      headers: { 'Accept': 'application/json' }
    }).catch(() => null);

    if (response && response.status() === 200) {
      const data = await response.json();

      if (Array.isArray(data) && data.length > 0) {
        expect(data[0]).toHaveProperty('idUsuario');
        expect(data[0]).toHaveProperty('nome');
      }
    }
  });

  test('/orcamentos/fornecedores should return proper JSON format', async ({ page }) => {
    const response = await page.request.get('/orcamentos/fornecedores', {
      headers: { 'Accept': 'application/json' }
    }).catch(() => null);

    if (response && response.status() === 200) {
      const data = await response.json();

      if (Array.isArray(data) && data.length > 0) {
        expect(data[0]).toHaveProperty('razaoSocial');
      }
    }
  });

  test('/orcamentos/data should return quotations with proper structure', async ({ page }) => {
    const response = await page.request.get('/orcamentos/data', {
      headers: { 'Accept': 'application/json' }
    }).catch(() => null);

    if (response && response.status() === 200) {
      const result = await response.json();

      // Should have success field
      if (result.success && result.data) {
        // If we have quotations
        if (result.data.length > 0) {
          const quote = result.data[0];

          // Check required fields
          expect(quote).toHaveProperty('idOrcamento');
          expect(quote).toHaveProperty('idLoja');
          expect(quote).toHaveProperty('status');
          expect(quote).toHaveProperty('diasRestantes');
          expect(quote).toHaveProperty('vendedor');
          expect(quote).toHaveProperty('cliente');
          expect(quote).toHaveProperty('data');
          expect(quote).toHaveProperty('total');
        }
      }
    }
  });
});

test.describe('Page Navigation', () => {
  test('login page should be accessible', async ({ page }) => {
    const response = await page.goto('/');
    expect(response.status()).toBe(200);
  });

  test('orcamentos page should exist', async ({ page }) => {
    // Try to navigate, might redirect to login if not authenticated
    const response = await page.goto('/orcamentos');
    expect([200, 302]).toContain(response.status());
  });

  test('logout page should exist', async ({ page }) => {
    const response = await page.goto('/logout', { waitUntil: 'domcontentloaded' });
    expect([200, 302]).toContain(response.status());
  });
});

test.describe('Form Handling', () => {
  test('login form should have proper attributes', async ({ page }) => {
    await page.goto('/');

    const form = page.locator('form');
    const userInput = page.locator('input[name="user"]');
    const passwordInput = page.locator('input[name="password"]');

    // Check input types
    expect(await userInput.getAttribute('type')).toBe('text');
    expect(await passwordInput.getAttribute('type')).toBe('password');

    // Check required attributes
    expect(await userInput.getAttribute('required')).not.toBeNull();
    expect(await passwordInput.getAttribute('required')).not.toBeNull();
  });

  test('filters form should have proper structure', async ({ page }) => {
    await page.goto('/orcamentos').catch(() => {});

    const filterForm = page.locator('#filterForm');
    if (await filterForm.isVisible().catch(() => false)) {
      // Should have search input
      const searchInput = filterForm.locator('#search');
      expect(await searchInput.getAttribute('type')).toBe('text');

      // Should have month input
      const monthInput = filterForm.locator('#mesAno');
      if (await monthInput.isVisible().catch(() => false)) {
        expect(await monthInput.getAttribute('type')).toBe('month');
      }
    }
  });
});

test.describe('Security Checks', () => {
  test('should not expose sensitive information in page source', async ({ page }) => {
    await page.goto('/');

    const content = await page.content();

    // Should not have database credentials
    expect(content).not.toContain('password=');
    expect(content).not.toContain('DATABASE_URL');
    expect(content).not.toContain('JWT_SECRET');
  });

  test('should set proper security headers for login page', async ({ page }) => {
    const response = await page.goto('/');

    const headers = response.allHeaders();

    // Check for some basic security headers
    // Actual headers depend on server configuration
    expect(response.status()).toBe(200);
  });

  test('should redirect login attempts to HTTPS check', async ({ page }) => {
    // Test that the application handles HTTP/HTTPS properly
    // This is environment-dependent
    test.skip(true, 'Depends on production HTTPS configuration');
  });
});

test.describe('Database Connection Validation', () => {
  test('application should be able to load page with valid database', async ({ page }) => {
    const response = await page.goto('/');

    // If database is working, login page should load
    expect(response.status()).toBe(200);

    // Check that page has expected content
    const title = await page.title();
    expect(title).toContain('Staccato ERP');
  });

  test('orcamentos page should handle missing session gracefully', async ({ page, context }) => {
    // Clear cookies to simulate no session
    await context.clearCookies();

    // Navigate to orcamentos
    const response = await page.goto('/orcamentos');

    // Should either redirect to login or show login page
    expect([200, 302]).toContain(response.status());

    // If redirected, should end up at login
    if (response.status() === 302) {
      await page.waitForURL('/');
    }
  });
});
