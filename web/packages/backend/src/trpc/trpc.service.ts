import { Injectable } from '@nestjs/common';
import { initTRPC, TRPCError } from '@trpc/server';
import type { SessionUser } from '@erp-staccato/shared';

export interface Context {
  user: SessionUser | null;
}

@Injectable()
export class TrpcService {
  trpc = initTRPC.context<Context>().create();

  router = this.trpc.router;
  procedure = this.trpc.procedure;
  middleware = this.trpc.middleware;

  // Protected procedure - requires authentication
  protectedProcedure = this.trpc.procedure.use(
    this.trpc.middleware(({ ctx, next }) => {
      if (!ctx.user) {
        throw new TRPCError({ code: 'UNAUTHORIZED' });
      }
      return next({
        ctx: {
          user: ctx.user,
        },
      });
    }),
  );
}
