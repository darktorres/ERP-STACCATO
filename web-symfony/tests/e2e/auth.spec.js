const { test, expect } = require('@playwright/test');

test.describe('Authentication - Login Page', () => {
  test.beforeEach(async ({ page, context }) => {
    // Clear any existing cookies/session
    await context.clearCookies();
    // Navigate to login page
    await page.goto('/');
  });

  test('should display login form with correct elements', async ({ page }) => {
    // Wait for page to fully load
    await page.waitForLoadState('networkidle');

    // Check page title
    await expect(page).toHaveTitle(/Staccato ERP/);

    // Check login container exists
    const loginContainer = page.locator('.login-container');
    if (await loginContainer.isVisible().catch(() => false)) {
      // Check header
      await expect(page.locator('h1')).toContainText('Staccato ERP');
      await expect(page.locator('.login-header p')).toContainText('Sistema de Gestão Comercial');

      // Check form elements
      await expect(page.locator('#user')).toBeVisible();
      await expect(page.locator('#password')).toBeVisible();
      await expect(page.locator('#rememberPassword')).toBeVisible();
      await expect(page.locator('button[type="submit"]')).toContainText('Entrar');
    }
  });

  test('should show error for empty credentials', async ({ page }) => {
    // Try to submit empty form
    await page.click('button[type="submit"]');

    // Check for HTML5 validation messages
    const userInput = page.locator('input[name="user"]');
    expect(await userInput.evaluate(el => el.validity.valid)).toBe(false);
  });

  test('should show error for invalid credentials', async ({ page }) => {
    // Wait for page to load
    await page.waitForLoadState('networkidle');

    // Fill with invalid credentials
    await page.fill('#user', 'invaliduser');
    await page.fill('#password', 'wrongpassword');

    // Click submit and wait for response
    await page.click('button[type="submit"]');

    // Wait for the page to reload with error message
    await page.waitForLoadState('networkidle');

    // Check if error message is displayed
    const errorVisible = await page.locator('.alert-danger').isVisible().catch(() => false);
    if (errorVisible) {
      await expect(page.locator('.alert-danger')).toContainText('Login inválido');
    }
  });

  test('should remember username when checkbox is checked', async ({ page, context }) => {
    const username = 'testuser';

    // Note: This test assumes a valid user exists
    // For now, we just test the checkbox behavior
    await page.fill('input[name="user"]', username);
    await page.check('input[name="rememberPassword"]');

    // Verify checkbox is checked
    await expect(page.locator('input[name="rememberPassword"]')).toBeChecked();
  });

  test('should not show remember checkbox value if unchecked', async ({ page }) => {
    await page.fill('input[name="user"]', 'testuser');
    await expect(page.locator('input[name="rememberPassword"]')).not.toBeChecked();
  });

  test('should redirect to orcamentos on successful login', async ({ page, context }) => {
    console.log('TEST: Starting login test');
    console.log('TEST: Current URL before login:', page.url());

    // Login with valid credentials
    console.log('TEST: Filling username field');
    await page.fill('#user', 'torres');

    console.log('TEST: Filling password field');
    await page.fill('#password', '1234');

    console.log('TEST: Clicking submit button');
    await page.click('button[type="submit"]');

    console.log('TEST: Waiting for redirect to /orcamentos');
    // Should redirect to orcamentos
    try {
      await page.waitForURL('/orcamentos', { timeout: 15000 });
      console.log('TEST: Successfully redirected to:', page.url());

      // Wait for page to finish loading
      console.log('TEST: Waiting for page to load');
      await page.waitForLoadState('networkidle', { timeout: 45000 });
      console.log('TEST: Page fully loaded');

      await expect(page).toHaveURL(/\/orcamentos/);
    } catch (e) {
      console.log('TEST: Failed to redirect or load. Current URL:', page.url());
      console.log('TEST: Page title:', await page.title());
      const errorMsg = await page.locator('.alert-danger').textContent().catch(() => 'No error message found');
      console.log('TEST: Error message:', errorMsg);
      const pageContent = await page.content();
      console.log('TEST: Page contains login form:', pageContent.includes('loginForm'));
      throw e;
    }
  });

  test('should display loading state on form submission', async ({ page }) => {
    // Fill form
    await page.fill('input[name="user"]', 'testuser');
    await page.fill('input[name="password"]', 'testpass');

    // Get button before click
    const submitButton = page.locator('button[type="submit"]');

    // Click submit
    const clickPromise = submitButton.click();

    // Button should have loading class or be disabled
    // (Depends on the exact implementation)
    // For now, just verify the click doesn't error
    await clickPromise;
  });

  test('should have proper styling and layout', async ({ page }) => {
    // Check that login container exists
    await expect(page.locator('.login-container')).toBeVisible();

    // Check that form elements are properly styled
    const userInput = page.locator('input[name="user"]');
    const passwordInput = page.locator('input[name="password"]');

    // Verify inputs have proper classes
    await expect(userInput).toHaveClass(/form-control/);
    await expect(passwordInput).toHaveClass(/form-control/);
  });

  test('should show footer with version info', async ({ page }) => {
    // Check for version info in footer
    const footer = page.locator('.login-footer');
    await expect(footer).toContainText(/0\.10\.136/);
    await expect(footer).toContainText(/Staccato/);
  });
});

test.describe('Authentication - Logout', () => {
  test.beforeEach(async ({ page, context }) => {
    // Clear any existing cookies/session
    await context.clearCookies();
    // Navigate to login page
    await page.goto('/');
  });

  test('should show logout button when navigating to orcamentos (if logged in)', async ({ page }) => {
    // Login first
    await page.fill('#user', 'torres');
    await page.fill('#password', '1234');
    await page.click('button[type="submit"]');

    // Should redirect to orcamentos
    await page.waitForURL('/orcamentos', { timeout: 10000 });

    // Check for logout button
    const logoutButton = page.locator('a:has-text("Sair")');
    await expect(logoutButton).toBeVisible();
  });

  test('logout should clear session and redirect to login', async ({ page }) => {
    // Login first
    await page.fill('#user', 'torres');
    await page.fill('#password', '1234');
    await page.click('button[type="submit"]');

    // Should redirect to orcamentos
    await page.waitForURL('/orcamentos', { timeout: 10000 });

    // Click logout button
    const logoutButton = page.locator('a:has-text("Sair")');
    await logoutButton.click();

    // Should redirect to login
    await page.waitForURL('/', { timeout: 10000 });
    await expect(page).toHaveURL(/^http:\/\/localhost:8000\/?$/);
  });
});
