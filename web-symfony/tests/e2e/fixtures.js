const { test as base } = require('@playwright/test');

/**
 * Custom fixtures for test setup/teardown
 */
const test = base.extend({
  /**
   * Clear session before each test
   */
  clearSession: async ({ context }, use) => {
    await context.clearCookies();
    await use(null);
  },

  /**
   * Login as a specific user
   */
  login: async ({ page }, use) => {
    const loginAs = async (username, password = 'senha') => {
      await page.goto('/');
      await page.fill('input[name="user"]', username);
      await page.fill('input[name="password"]', password);
      await page.click('button[type="submit"]');
      await page.waitForURL('/orcamentos', { timeout: 10000 });
      return page;
    };

    await use(loginAs);
  },
});

module.exports = { test };
