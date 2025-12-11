import { createTRPCReact } from '@trpc/react-query';
import { httpBatchLink } from '@trpc/client';
import type { AppRouter } from '@erp-staccato/backend/src/trpc/trpc.router';

export const trpc = createTRPCReact<AppRouter>();

// Get auth token from localStorage
const getAuthToken = () => {
  if (typeof window !== 'undefined') {
    return localStorage.getItem('auth-token');
  }
  return null;
};

export const trpcClient = trpc.createClient({
  links: [
    httpBatchLink({
      url: 'http://localhost:3001/trpc',
      headers() {
        const token = getAuthToken();
        return token
          ? {
              Authorization: `Bearer ${token}`,
            }
          : {};
      },
      async fetch(url, options) {
        const requestTime = performance.now();
        const body = options?.body;
        let procedureName = 'unknown';

        // Try to extract procedure name from body
        if (body instanceof FormData) {
          // Handle FormData if needed
        } else if (typeof body === 'string') {
          try {
            const parsed = JSON.parse(body);
            if (Array.isArray(parsed)) {
              procedureName = parsed[0]?.meta?.path?.join('.') || 'batch';
            }
          } catch (e) {
            // Ignore parsing errors
          }
        }

        console.log(`[tRPC] → ${procedureName} (request sent)`);

        const response = await fetch(url, options);
        const responseTime = performance.now();
        const duration = responseTime - requestTime;

        console.log(`[tRPC] ← ${procedureName} (received in ${duration.toFixed(2)}ms)`);

        return response;
      },
    }),
  ],
});
