import { NestFactory } from '@nestjs/core';
import { FastifyAdapter, NestFastifyApplication } from '@nestjs/platform-fastify';
import { AppModule } from './app.module.js';
import { TrpcRouter } from './trpc/trpc.router.js';
import { createHTTPHandler } from '@trpc/server/adapters/standalone';

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

  // Register tRPC routes after app initialization
  const fastify = app.getHttpAdapter().getInstance();

  fastify.post('/trpc', async (request: any, reply: any) => {
    try {
      // Get the tRPC router instance (lazy load to ensure it's initialized)
      const trpcRouter = app.get(TrpcRouter);

      // Mount tRPC handler
      const trpcHandler = createHTTPHandler({
        router: trpcRouter.appRouter,
        createContext: async () => ({
          user: (request as any).user || null,
        }),
      });

      await trpcHandler(
        {
          method: request.method,
          headers: request.headers,
          query: request.query,
          body: request.body,
        } as any,
        {
          status: (code: number) => {
            reply.status(code);
            return reply;
          },
          setHeader: (key: string, value: any) => {
            reply.header(key, value);
            return reply;
          },
          end: (data: any) => {
            reply.send(data);
          },
          write: (data: any) => {
            reply.write(data);
          },
        } as any,
      );
    } catch (error) {
      console.error('tRPC error:', error);
      reply.status(500).send({ error: 'Internal Server Error' });
    }
  });

  await app.listen(port, '0.0.0.0');
  console.log(`Backend running on http://localhost:${port}`);
}

bootstrap();
