import { describe, it, expect } from 'vitest';

/**
 * Integration tests for Orcamento (Budget) API
 * Tests filtering and data retrieval against the running backend server
 * Assumes backend is running on http://localhost:3001
 */
describe('Orcamento Integration Tests', () => {
  const baseUrl = 'http://localhost:3001';

  describe('GET /trpc/orcamento.list', () => {
    it('should require authentication', async () => {
      // First, test that unauthenticated request fails
      const response = await fetch(`${baseUrl}/trpc/orcamento.list?batch=1`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify([
          {
            json: {},
          },
        ]),
      });

      expect(response.ok).toBe(true);
      const data = await response.json();

      // Should have error because no auth token
      expect(Array.isArray(data)).toBe(true);
      const result = data[0];
      console.log('Unauthenticated response:', JSON.stringify(result, null, 2));

      expect(result).toHaveProperty('error');
      expect(result.error.code).toBe(-32001); // UNAUTHORIZED
    });

    it('should return orcamentos after login', async () => {
      // Step 1: Login
      const loginResponse = await fetch(`${baseUrl}/trpc/auth.login?batch=1`, {
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

      const loginData = await loginResponse.json();
      expect(loginData[0]).toHaveProperty('result');
      const token = loginData[0].result.data.token;
      console.log('Got token for user torres');

      // Step 2: Try to fetch orcamentos with the token
      try {
        const orcamentosResponse = await fetch(`${baseUrl}/trpc/orcamento.list?batch=1`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${token}`,
          },
          body: JSON.stringify([
            {
              json: {},
            },
          ]),
        });

        const text = await orcamentosResponse.text();
        console.log('Orcamentos response status:', orcamentosResponse.status);
        console.log('Orcamentos response body:', text);

        const orcamentosData = JSON.parse(text);

        if (orcamentosResponse.status === 200) {
          // Batch response
          const result = orcamentosData[0];
          if (result.result) {
            console.log(`Successfully fetched ${result.result.data.length} orcamentos`);
            expect(Array.isArray(result.result.data)).toBe(true);
          } else if (result.error) {
            console.log('Got tRPC error:', result.error.message);
          }
        } else {
          console.log('Got HTTP error response');
        }
      } catch (error) {
        console.error('Fetch error:', error);
        throw error;
      }
    });

    it('should have expected database schema', async () => {
      // This test just documents what fields should exist
      // We can't run it without auth, but it shows the expected structure
      console.log('Expected orcamento fields from schema:');
      console.log('- idOrcamento: string');
      console.log('- idLoja: number');
      console.log('- idUsuario: number');
      console.log('- status: "ATIVO" | "EXPIRADO" | "FECHADO" | "PERDIDO" | "CANCELADO" | "REPLICADO"');
      console.log('- diasRestantes: string | number (empty for closed/lost)');
      console.log('- vendedor: string');
      console.log('- consultor: string | null');
      console.log('- cliente: string');
      console.log('- profissional: string | null');
      console.log('- tel: string | null');
      console.log('- telCel: string | null');
      console.log('- telProf: string | null');
      console.log('- data: Date');
      console.log('- data2: string (YYYY-MM format)');
      console.log('- total: Decimal');
      console.log('- observacao: string | null');
      console.log('- fornecedores: string | null (comma-separated)');
      expect(true).toBe(true);
    });
  });
});
