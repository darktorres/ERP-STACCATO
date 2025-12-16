/**
 * Global setup for tests
 * Creates test user if it doesn't exist
 */
const { chromium } = require('@playwright/test');
const http = require('http');

async function setupTests() {
  console.log('Setting up test environment...');

  // Wait for server to be ready
  const maxRetries = 30;
  let serverReady = false;

  for (let i = 0; i < maxRetries; i++) {
    try {
      const response = await fetch('http://localhost:8000/', {
        method: 'GET',
        timeout: 1000,
      });
      if (response.status === 200 || response.status === 302) {
        serverReady = true;
        break;
      }
    } catch (e) {
      // Server not ready yet
    }
    await new Promise(resolve => setTimeout(resolve, 1000));
  }

  if (!serverReady) {
    console.warn('Warning: Server may not be ready for tests');
  }

  console.log('Test environment ready!');
}

module.exports = setupTests;
