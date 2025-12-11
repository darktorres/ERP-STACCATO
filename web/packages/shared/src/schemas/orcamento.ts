import { z } from 'zod';

export const orcamentoStatusSchema = z.enum([
  'ATIVO',
  'EXPIRADO',
  'FECHADO',
  'PERDIDO',
  'CANCELADO',
  'REPLICADO',
]);

export const orcamentoFiltersSchema = z.object({
  idLoja: z.number().optional(),
  idVendedor: z.number().optional(),
  fornecedor: z.string().optional(),
  statuses: z.array(orcamentoStatusSchema).optional(),
  mesAno: z.string().regex(/^\d{4}-\d{2}$/).optional(),
  semaforo: z.number().min(1).max(3).optional(),
  search: z.string().optional(),
  apenasPropriosOrcamentos: z.boolean().optional(),
});
