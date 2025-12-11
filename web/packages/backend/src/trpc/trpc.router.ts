import { Injectable, OnModuleInit } from '@nestjs/common';
import { TRPCError } from '@trpc/server';
import { z, ZodError } from 'zod';
import { TrpcService, zodErrorToDetails } from './trpc.service.js';
import { AuthService } from '../modules/auth/auth.service.js';
import { OrcamentoService } from '../modules/orcamento/orcamento.service.js';
import { loginSchema, authorizationSchema, orcamentoFiltersSchema } from '@erp-staccato/shared';

@Injectable()
export class TrpcRouter implements OnModuleInit {
  public appRouter!: ReturnType<typeof this.createRouter>;

  constructor(
    private trpc: TrpcService,
    private authService: AuthService,
    private orcamentoService: OrcamentoService,
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
        login: this.trpc.procedure
          .input(loginSchema)
          .use(async (opts) => {
            // Middleware to catch and properly format input validation errors
            try {
              return await opts.next();
            } catch (error) {
              // tRPC throws errors with stringified Zod issues in the message
              // e.g., "[{code: "invalid_type", path: ["password"], ...}]"
              if (error instanceof TRPCError && error.message.startsWith('[')) {
                try {
                  const zodIssues = JSON.parse(error.message);
                  if (Array.isArray(zodIssues) && zodIssues[0]?.code) {
                    const fieldErrors: Record<string, string[]> = {};
                    for (const issue of zodIssues) {
                      const path = issue.path?.join('.') || 'unknown';
                      if (!fieldErrors[path]) {
                        fieldErrors[path] = [];
                      }
                      fieldErrors[path].push(issue.message);
                    }
                    throw new TRPCError({
                      code: 'BAD_REQUEST',
                      message: 'Validation error',
                      cause: { code: 'BAD_REQUEST', fieldErrors },
                    });
                  }
                } catch (parseError) {
                  // If not a Zod error, re-throw original
                  throw error;
                }
              }
              throw error;
            }
          })
          .mutation(async ({ input }) => {
            try {
              return await this.authService.login(input);
            } catch (error) {
              if (error instanceof ZodError) {
                // Validation error - include field-level details
                const details = zodErrorToDetails(error);
                throw new TRPCError({
                  code: 'BAD_REQUEST',
                  message: details.message,
                  cause: details,
                });
              }
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

      orcamento: this.trpc.router({
        /**
         * List orcamentos with filtering
         * Requires authentication
         */
        list: this.trpc.protectedProcedure
          .input(orcamentoFiltersSchema)
          .query(async ({ input, ctx }) => {
            try {
              return await this.orcamentoService.list(
                input,
                ctx.user.idUsuario,
                ctx.user.tipo,
                ctx.user.idLoja,
                ctx.user.nome,
              );
            } catch (error) {
              if (error instanceof Error) {
                throw new TRPCError({
                  code: 'INTERNAL_SERVER_ERROR',
                  message: error.message,
                });
              }
              throw new TRPCError({
                code: 'INTERNAL_SERVER_ERROR',
                message: 'Erro ao listar orçamentos',
              });
            }
          }),

        /**
         * Get lojas for filter dropdown
         */
        lojas: this.trpc.protectedProcedure.query(async () => {
          try {
            return await this.orcamentoService.getLojasForFilter();
          } catch (error) {
            throw new TRPCError({
              code: 'INTERNAL_SERVER_ERROR',
              message: 'Erro ao carregar lojas',
            });
          }
        }),

        /**
         * Get vendedores for filter dropdown
         */
        vendedores: this.trpc.protectedProcedure
          .input(z.object({ idLoja: z.number().optional() }))
          .query(async ({ input }) => {
            try {
              return await this.orcamentoService.getVendedoresForFilter(input.idLoja);
            } catch (error) {
              throw new TRPCError({
                code: 'INTERNAL_SERVER_ERROR',
                message: 'Erro ao carregar vendedores',
              });
            }
          }),

        /**
         * Get fornecedores for filter dropdown
         */
        fornecedores: this.trpc.protectedProcedure.query(async () => {
          try {
            return await this.orcamentoService.getFornecedoresForFilter();
          } catch (error) {
            throw new TRPCError({
              code: 'INTERNAL_SERVER_ERROR',
              message: 'Erro ao carregar fornecedores',
            });
          }
        }),
      }),
    });
  }
}

export type AppRouter = ReturnType<TrpcRouter['createRouter']>;
