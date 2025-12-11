import { Injectable } from '@nestjs/common';
import { initTRPC, TRPCError } from '@trpc/server';
import type { SessionUser } from '@erp-staccato/shared';
import { ZodError } from 'zod';

export interface Context {
  user: SessionUser | null;
}

/**
 * Convert Zod validation errors to structured error details
 * Extracts field-specific errors in a format that can be shown to users
 */
export function zodErrorToDetails(error: ZodError) {
  const fieldErrors: Record<string, string[]> = {};

  for (const issue of error.issues) {
    const path = issue.path.join('.');
    if (!fieldErrors[path]) {
      fieldErrors[path] = [];
    }
    fieldErrors[path].push(issue.message);
  }

  return {
    code: 'BAD_REQUEST',
    message: 'Validation error',
    fieldErrors,
  };
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
