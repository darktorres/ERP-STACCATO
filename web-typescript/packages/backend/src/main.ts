import { NestFactory } from '@nestjs/core';
import { FastifyAdapter, NestFastifyApplication } from '@nestjs/platform-fastify';
import { JwtService } from '@nestjs/jwt';
import { writeFileSync, appendFileSync } from 'fs';
import { AppModule } from './app.module.js';
import { TrpcRouter } from './trpc/trpc.router.js';

// Log file for debugging
import { resolve } from 'path';
const LOG_FILE = resolve(process.cwd(), 'trpc-debug.log');

function debugLog(label: string, data: any) {
  try {
    const timestamp = new Date().toISOString();
    const message = `[${timestamp}] ${label}: ${JSON.stringify(data)}\n`;
    appendFileSync(LOG_FILE, message);
  } catch (e) {
    // Silently fail if can't write
  }
}

/**
 * Register tRPC routes on the Fastify instance
 * Extracted to be reusable from tests
 */
export async function registerTrpcRoutes(app: NestFastifyApplication) {
  console.log('[STARTUP] Registering tRPC routes...');
  const fastify = app.getHttpAdapter().getInstance();

  // Extract JWT service once on startup
  let jwtService: JwtService | null = null;
  try {
    jwtService = app.get(JwtService);
  } catch (error) {
    console.warn('JwtService not available in tRPC handler');
  }

  // Handle tRPC requests directly
  const trpcHandler = async (request: any, reply: any) => {
    try {
      console.log('[tRPC] Incoming request:', {
        method: request.method,
        url: request.url,
        hasAuth: !!request.headers.authorization,
        hasBody: !!request.body,
      });

      // Get the tRPC router instance
      console.log('[tRPC] Getting TrpcRouter from DI container...');
      let trpcRouter: TrpcRouter;
      try {
        trpcRouter = app.get(TrpcRouter);
      } catch (error) {
        console.error('[tRPC] Failed to get TrpcRouter:', error instanceof Error ? error.message : error);
        throw error;
      }
      console.log('[tRPC] TrpcRouter obtained');
      const router = trpcRouter.appRouter;

      // Parse the request
      const method = request.method as 'GET' | 'POST' | 'PUT' | 'DELETE';
      const url = new URL(request.url, `http://${request.headers.host}`);
      const pathname = url.pathname;

      // Extract the procedure path from /trpc/path/to/procedure
      const procedurePath = pathname.replace(/^\/trpc\/?/, '').split('?')[0];
      console.log('[tRPC] Procedure path:', procedurePath);

      // Parse query parameters
      const isBatch = url.searchParams.has('batch');
      let requestData: any = {};

      if (method === 'POST' && request.body) {
        requestData = request.body;
      } else if (method === 'GET' && url.searchParams.has('input')) {
        // For GET requests, parse the input from query parameters
        const inputParam = url.searchParams.get('input');
        if (inputParam) {
          try {
            requestData = JSON.parse(inputParam);
            console.log('[tRPC] Parsed GET input from query params:', inputParam.substring(0, 100));
          } catch (e) {
            console.error('[tRPC] Failed to parse input query parameter:', inputParam);
          }
        }
      }
      console.log('[tRPC] Request data:', JSON.stringify(requestData).substring(0, 200));

      // Extract JWT from Authorization header
      let user: any = null;
      const authHeader = request.headers.authorization;
      console.log('[tRPC] Auth header present:', !!authHeader);
      console.log('[tRPC] JwtService available:', !!jwtService);

      if (authHeader && authHeader.startsWith('Bearer ') && jwtService) {
        const token = authHeader.substring(7);
        console.log('[tRPC] Attempting JWT verification...');
        try {
          const payload = jwtService.verify(token);
          user = payload;
          console.log('[tRPC] JWT verified successfully, user:', {
            idUsuario: user?.idUsuario,
            tipo: user?.tipo,
          });
        } catch (error) {
          // Invalid token - JWT verification failed
          console.error('[tRPC] JWT verification failed:', error instanceof Error ? error.message : error);
          // user stays null, protected procedures will fail with UNAUTHORIZED
        }
      } else {
        console.log('[tRPC] Skipping JWT verification - no auth header or no JwtService');
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
          console.log('[tRPC] Processing request:', { procedurePath, hasInput: !!req.input });

          // Call the procedure directly from the router
          console.log('[tRPC] Creating caller...');
          const caller = router.createCaller({
            user: user,
          });
          console.log('[tRPC] Caller created successfully');

          // Parse the procedure path and call it
          console.log('[tRPC] Parsing procedure path:', procedurePath);
          const parts = procedurePath.split('.');
          let procedure: any = caller;

          for (const part of parts) {
            if (!part) continue;
            console.log('[tRPC] Resolving procedure part:', part);
            procedure = procedure[part];
            if (!procedure) {
              console.error('[tRPC] Procedure not found:', procedurePath);
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
          console.log('[tRPC] Calling procedure with method:', method, 'input:', JSON.stringify(req.input).substring(0, 100));
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

          console.log('[tRPC] Procedure result received, array length:', Array.isArray(result) ? result.length : 'not array');
          responses.push({ result: { data: result } });
        } catch (error) {
          const procError = {
            message: error instanceof Error ? error.message : String(error),
            code: error instanceof Error && 'code' in error ? (error as any).code : 'INTERNAL_SERVER_ERROR',
            stack: error instanceof Error ? error.stack?.split('\n').slice(0, 10).join('\n') : undefined,
            procedurePath,
            type: error?.constructor?.name,
          };
          debugLog('tRPC procedure execution error', procError);
          console.error('[tRPC] Procedure execution error:', JSON.stringify(procError, null, 2));
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
      const errorObj = {
        message: error instanceof Error ? error.message : String(error),
        stack: error instanceof Error ? error.stack?.split('\n').slice(0, 5).join('\n') : undefined,
        type: error?.constructor?.name,
      };
      debugLog('tRPC handler unhandled error', errorObj);
      console.error('[tRPC] Unhandled handler error:', errorObj);
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

  // Add error handler to log errors before sending 500 response
  fastify.setErrorHandler((error: any, request, reply) => {
    const errorInfo = {
      statusCode: error?.statusCode,
      message: error?.message,
      url: request.url,
      method: request.method,
      stack: error?.stack?.split('\n').slice(0, 5).join('\n'),
    };
    // Log to Fastify's logger (console)
    fastify.log.error(errorInfo, 'Request error handler caught exception');
    debugLog('Fastify error handler', errorInfo);
    reply.status(error?.statusCode || 500).send({
      statusCode: error?.statusCode || 500,
      message: error?.message || 'Internal server error',
    });
  });

  // Register route to catch all /trpc requests
  console.log('[STARTUP] Registering /trpc route...');
  fastify.all('/trpc', trpcHandler);
  console.log('[STARTUP] Registered /trpc');
  fastify.all('/trpc/*', trpcHandler);
  console.log('[STARTUP] Registered /trpc/*');
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

  // Add a hook to log all requests
  const fastify = app.getHttpAdapter().getInstance();
  fastify.addHook('onError', async (request, reply, error) => {
    const errorInfo = {
      timestamp: new Date().toISOString(),
      method: request.method,
      url: request.url,
      statusCode: reply.statusCode,
      message: error.message,
      stack: error.stack?.split('\n').slice(0, 5).join('\n'),
    };
    console.error('[Fastify] Error:', errorInfo);
    debugLog('Fastify onError hook', errorInfo);
  });

  await app.listen(port, '0.0.0.0');
  console.log(`Backend running on http://localhost:${port}`);
}

bootstrap();
