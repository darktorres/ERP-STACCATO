import { Injectable, OnModuleInit } from '@nestjs/common';
import { TRPCError } from '@trpc/server';
import { TrpcService } from './trpc.service.js';
import { AuthService } from '../modules/auth/auth.service.js';
import { loginSchema, authorizationSchema } from '@erp-staccato/shared';

@Injectable()
export class TrpcRouter implements OnModuleInit {
  public appRouter!: ReturnType<typeof this.createRouter>;

  constructor(
    private trpc: TrpcService,
    private authService: AuthService,
  ) {}

  onModuleInit() {
    this.appRouter = this.createRouter();
  }

  private createRouter() {
    return this.trpc.router({
      auth: this.trpc.router({
        /**
         * Login procedure
         * Matches C++ LoginDialog::on_pushButtonLogin_clicked behavior
         */
        login: this.trpc.procedure.input(loginSchema).mutation(async ({ input }) => {
          try {
            return await this.authService.login(input);
          } catch (error) {
            if (error instanceof Error) {
              throw new TRPCError({
                code: 'UNAUTHORIZED',
                message: error.message,
              });
            }
            throw new TRPCError({
              code: 'INTERNAL_SERVER_ERROR',
              message: 'Erro no login',
            });
          }
        }),

        /**
         * Authorization procedure (one-time password)
         * Matches C++ User::autorizacao behavior
         */
        authorize: this.trpc.procedure
          .input(authorizationSchema)
          .mutation(async ({ input }) => {
            try {
              return await this.authService.authorize(input);
            } catch (error) {
              if (error instanceof Error) {
                throw new TRPCError({
                  code: 'UNAUTHORIZED',
                  message: error.message,
                });
              }
              throw new TRPCError({
                code: 'INTERNAL_SERVER_ERROR',
                message: 'Erro na autorização',
              });
            }
          }),

        /**
         * Get current user (requires authentication)
         */
        me: this.trpc.protectedProcedure.query(({ ctx }) => {
          return ctx.user;
        }),
      }),
    });
  }
}

export type AppRouter = ReturnType<TrpcRouter['createRouter']>;
