import { describe, it, expect, beforeAll, afterAll } from 'vitest';
import { NestFactory } from '@nestjs/core';
import { FastifyAdapter, NestFastifyApplication } from '@nestjs/platform-fastify';
import { AppModule } from '../../app.module.js';
import { registerTrpcRoutes } from '../../main.js';

/**
 * Integration tests for Authentication (Login)
 * Tests the full tRPC login endpoint without mocks
 */
describe('Auth Integration Tests', () => {
  let app: NestFastifyApplication;
  const testPort = 3007; // Use a unique test port
  const baseUrl = `http://localhost:${testPort}`;

  beforeAll(async () => {
    // Create NestJS app for testing
    app = await NestFactory.create<NestFastifyApplication>(
      AppModule,
      new FastifyAdapter(),
    );

    app.enableCors({
      origin: ['*'],
      credentials: true,
    });

    // Register tRPC routes
    await registerTrpcRoutes(app);

    // Listen on a test port that doesn't conflict with dev server
    await app.listen(testPort, '127.0.0.1');
    console.log(`Test server running on ${baseUrl}`);

    // Give server time to start
    await new Promise(resolve => setTimeout(resolve, 500));
  });

  afterAll(async () => {
    if (app) {
      await app.close();
    }
  });

  describe('POST /trpc/auth.login', () => {
    it('should return success with valid torres credentials', async () => {
      const response = await fetch(`${baseUrl}/trpc/auth.login?batch=1`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify([
          {
            json: {
              user: 'torres',
              password: '1234',
              staging: false,
            },
          },
        ]),
      });

      expect(response.ok).toBe(true);
      const data = await response.json();

      // Should be an array (batch response)
      expect(Array.isArray(data)).toBe(true);
      expect(data).toHaveLength(1);

      const result = data[0];
      expect(result).toHaveProperty('result');
      expect(result.result).toHaveProperty('data');

      const loginData = result.result.data;
      expect(loginData.success).toBe(true);
      expect(loginData).toHaveProperty('token');
      expect(typeof loginData.token).toBe('string');
      expect(loginData.token.split('.').length).toBe(3); // JWT format

      expect(loginData).toHaveProperty('user');
      expect(loginData.user).toMatchObject({
        user: 'torres',
        tipo: 'ADMINISTRADOR',
        nome: 'RODRIGO TORRES',
        idUsuario: expect.any(Number),
        idLoja: expect.any(Number),
      });

      expect(loginData.user).toHaveProperty('loja');
      expect(loginData.user.loja).toHaveProperty('idLoja');
      expect(loginData.user.loja).toHaveProperty('nomeFantasia');
    });

    it('should return error with invalid credentials', async () => {
      const response = await fetch(`${baseUrl}/trpc/auth.login?batch=1`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify([
          {
            json: {
              user: 'nonexistent',
              password: 'wrongpassword',
              staging: false,
            },
          },
        ]),
      });

      expect(response.ok).toBe(true);
      const data = await response.json();

      expect(Array.isArray(data)).toBe(true);
      const result = data[0];
      expect(result).toHaveProperty('error');
      expect(result.error).toHaveProperty('code');
      expect(result.error.code).toBe('UNAUTHORIZED');
      expect(result.error.message).toContain('Login inválido');
    });

    it('should return error with missing fields', async () => {
      const response = await fetch(`${baseUrl}/trpc/auth.login?batch=1`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify([
          {
            json: {
              user: 'torres',
              // missing password and staging
            },
          },
        ]),
      });

      expect(response.ok).toBe(true);
      const data = await response.json();

      const result = data[0];
      expect(result).toHaveProperty('error');
      expect(result.error.code).toBe('BAD_REQUEST');
    });

    it('should handle batch multiple requests', async () => {
      const response = await fetch(`${baseUrl}/trpc/auth.login?batch=1`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify([
          {
            json: {
              user: 'torres',
              password: '1234',
              staging: false,
            },
          },
          {
            json: {
              user: 'invalid',
              password: 'wrong',
              staging: false,
            },
          },
        ]),
      });

      expect(response.ok).toBe(true);
      const data = await response.json();

      expect(Array.isArray(data)).toBe(true);
      expect(data).toHaveLength(2);

      // First request should succeed
      expect(data[0]).toHaveProperty('result');
      expect(data[0].result.data.success).toBe(true);

      // Second request should fail
      expect(data[1]).toHaveProperty('error');
      expect(data[1].error.code).toBe('UNAUTHORIZED');
    });
  });
});
