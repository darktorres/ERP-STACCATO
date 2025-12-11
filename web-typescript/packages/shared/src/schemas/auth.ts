import { z } from 'zod';

// Login request schema - matches LoginDialog inputs
export const loginSchema = z.object({
  user: z
    .string()
    .min(1, 'Usuário é obrigatório')
    .max(20, 'Usuário deve ter no máximo 20 caracteres')
    .transform((val) => val.toLowerCase()),
  password: z.string().min(1, 'Senha é obrigatória'),
  staging: z.boolean(),
});

// Input type for forms (what goes in)
export type LoginInput = z.input<typeof loginSchema>;

// Output type after validation (what comes out)
export type LoginOutput = z.output<typeof loginSchema>;

// Authorization request schema - for one-time password authorization
export const authorizationSchema = z.object({
  user: z.string().min(1, 'Usuário é obrigatório'),
  senhaUsoUnico: z.string().length(4, 'Senha de uso único deve ter 4 caracteres'),
});

export type AuthorizationInput = z.infer<typeof authorizationSchema>;

// Detailed error response with field-specific validation errors
export const errorDetailsSchema = z.object({
  code: z.string(),
  message: z.string(),
  // Field-specific validation errors: { fieldName: ["error1", "error2"] }
  fieldErrors: z.record(z.array(z.string())).optional(),
  // Additional context (path, timestamp, etc.)
  context: z.record(z.unknown()).optional(),
});

export type ErrorDetails = z.infer<typeof errorDetailsSchema>;

// Login response
export const loginResponseSchema = z.object({
  success: z.boolean(),
  token: z.string().optional(),
  user: z
    .object({
      idUsuario: z.number(),
      idLoja: z.number(),
      user: z.string(),
      tipo: z.string(),
      nome: z.string(),
      loja: z
        .object({
          idLoja: z.number(),
          descricao: z.string(),
          nomeFantasia: z.string(),
        })
        .optional(),
    })
    .optional(),
  error: z.string().optional(),
});

export type LoginResponse = z.infer<typeof loginResponseSchema>;
