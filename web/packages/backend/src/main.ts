import { NestFactory } from '@nestjs/core';
import { FastifyAdapter, NestFastifyApplication } from '@nestjs/platform-fastify';
import { JwtService } from '@nestjs/jwt';
import { AppModule } from './app.module.js';
import { TrpcRouter } from './trpc/trpc.router.js';

/**
 * Register tRPC routes on the Fastify instance
 * Extracted to be reusable from tests
 */
export async function registerTrpcRoutes(app: NestFastifyApplication) {
  const fastify = app.getHttpAdapter().getInstance();

  // Handle tRPC requests directly
  const trpcHandler = async (request: any, reply: any) => {
    try {
      // Get the tRPC router instance
      const trpcRouter = app.get(TrpcRouter);
      const router = trpcRouter.appRouter;

      // Parse the request
      const method = request.method as 'GET' | 'POST' | 'PUT' | 'DELETE';
      const url = new URL(request.url, `http://${request.headers.host}`);
      const pathname = url.pathname;

      // Extract the procedure path from /trpc/path/to/procedure
      const procedurePath = pathname.replace(/^\/trpc\/?/, '').split('?')[0];

      // Parse query parameters
      const isBatch = url.searchParams.has('batch');
      let requestData: any = {};

      if (method === 'POST' && request.body) {
        requestData = request.body;
      }

      // Extract JWT from Authorization header
      let user: any = null;
      const authHeader = request.headers.authorization;
      if (authHeader && authHeader.startsWith('Bearer ')) {
        const token = authHeader.substring(7);
        try {
          const jwtService = app.get(JwtService);
          const payload = jwtService.verify(token);
          user = payload;
        } catch (error) {
          // Invalid token - JWT verification failed
          // Log the error for debugging but don't crash
          console.error('JWT verification failed:', error instanceof Error ? error.message : error);
          // user stays null, protected procedures will fail with UNAUTHORIZED
        }
      }

      // Handle both batch and non-batch requests
      // Batch requests come as an array or as an object with numeric keys (sparse array from JSON)
      let requests: any[] = [];

      if (Array.isArray(requestData)) {
        // Standard batch format: [{json: {...}}, {json: {...}}]
        requests = requestData.map((item: any) => ({
          input: item.json,
          meta: item,
        }));
      } else if (typeof requestData === 'object' && requestData !== null) {
        // Check if it looks like an array with numeric keys
        const keys = Object.keys(requestData);
        const hasNumericKeys = keys.some((k: string) => !isNaN(Number(k)));

        if (hasNumericKeys && isBatch) {
          // Convert sparse array format ({"0": {...}, "1": {...}}) to proper format
          const items = Object.values(requestData);
          requests = (items as any[]).map((item: any) => ({
            input: item.json !== undefined ? item.json : item,
            meta: item,
          }));
        } else {
          // Single request: {json: {...}} or just the input object
          const input = requestData.json !== undefined ? requestData.json : requestData;
          requests = [{ input, meta: requestData }];
        }
      } else {
        requests = [{ input: requestData, meta: requestData }];
      }


      const responses = [];

      for (const req of requests) {
        try {
          // Call the procedure directly from the router
          const caller = router.createCaller({
            user: user,
          });

          // Parse the procedure path and call it
          const parts = procedurePath.split('.');
          let procedure: any = caller;

          for (const part of parts) {
            if (!part) continue;
            procedure = procedure[part];
            if (!procedure) {
              responses.push({
                error: {
                  code: 'NOT_FOUND',
                  message: `Procedure ${procedurePath} not found`,
                },
              });
              continue;
            }
          }

          // Determine if it's a query or mutation and call the procedure
          let result: any;
          if (method === 'GET') {
            result = await procedure(req.input);
          } else if (method === 'POST') {
            result = await procedure(req.input);
          } else {
            responses.push({
              error: {
                code: 'METHOD_NOT_ALLOWED',
                message: 'Method not allowed',
              },
            });
            continue;
          }

          responses.push({ result: { data: result } });
        } catch (error) {
          const message = error instanceof Error ? error.message : 'Internal Server Error';
          const stringCode = error instanceof Error && 'code' in error ? (error as any).code : 'INTERNAL_SERVER_ERROR';
          const errorCause = error instanceof Error && 'cause' in error ? (error as any).cause : null;

          // Map tRPC error codes to JSON-RPC 2.0 numeric codes
          const TRPC_ERROR_CODES_BY_KEY: Record<string, number> = {
            PARSE_ERROR: -32700,
            BAD_REQUEST: -32600,
            UNAUTHORIZED: -32001,
            FORBIDDEN: -32003,
            NOT_FOUND: -32004,
            TIMEOUT: -32008,
            CONFLICT: -32009,
            PRECONDITION_FAILED: -32012,
            PAYLOAD_TOO_LARGE: -32013,
            METHOD_NOT_SUPPORTED: -32005,
            UNPROCESSABLE_CONTENT: -32022,
            INTERNAL_SERVER_ERROR: -32603,
          };

          const numericCode = TRPC_ERROR_CODES_BY_KEY[stringCode] ?? -32603;

          // Return error response in tRPC format per JSON-RPC 2.0 spec
          // error.code MUST be a number, with string code in error.data.code
          const errorData: any = {
            code: stringCode,
          };

          // Include validation error details if available
          if (errorCause && typeof errorCause === 'object') {
            if ('fieldErrors' in errorCause) {
              errorData.fieldErrors = (errorCause as any).fieldErrors;
            }
            if ('context' in errorCause) {
              errorData.context = (errorCause as any).context;
            }
          }

          // Also try to extract fieldErrors from the message if it's a Zod validation error (stringified JSON)
          if (stringCode === 'BAD_REQUEST' && message.startsWith('[')) {
            try {
              const zodIssues = JSON.parse(message);
              if (Array.isArray(zodIssues) && zodIssues[0]?.code && !errorData.fieldErrors) {
                const fieldErrors: Record<string, string[]> = {};
                for (const issue of zodIssues) {
                  const path = issue.path?.join('.') || 'unknown';
                  if (!fieldErrors[path]) {
                    fieldErrors[path] = [];
                  }
                  fieldErrors[path].push(issue.message);
                }
                errorData.fieldErrors = fieldErrors;
              }
            } catch (e) {
              // Not a Zod error, ignore
            }
          }

          const errorShape = {
            code: numericCode,
            message,
            data: errorData,
          };

          responses.push({
            error: errorShape,
          });
        }
      }

      // Send response in tRPC format
      // For batch requests, return array; for single requests, return object
      // Always return 200 OK for tRPC responses (errors are in the response body)

      let responseToSend: any;
      if (isBatch) {
        responseToSend = responses;
      } else {
        responseToSend = responses[0];
      }

      reply.code(200).send(responseToSend);
    } catch (error) {
      console.error('tRPC handler error:', error);
      const message = error instanceof Error ? error.message : 'Internal Server Error';
      reply.status(500).send(
        JSON.stringify({
          error: {
            code: 'INTERNAL_SERVER_ERROR',
            message,
          },
        })
      );
    }
  };

  // Register route to catch all /trpc requests
  fastify.all('/trpc', trpcHandler);
  fastify.all('/trpc/*', trpcHandler);
}

async function bootstrap() {
  const app = await NestFactory.create<NestFastifyApplication>(
    AppModule,
    new FastifyAdapter(),
  );

  // Enable CORS for frontend
  app.enableCors({
    origin: ['http://localhost:5173', 'http://localhost:3000'],
    credentials: true,
  });

  const port = process.env.PORT || 3001;

  // Register tRPC routes
  await registerTrpcRoutes(app);

  await app.listen(port, '0.0.0.0');
  console.log(`Backend running on http://localhost:${port}`);
}

bootstrap();
