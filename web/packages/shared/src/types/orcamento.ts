// Orcamento (Budget) types
export type OrcamentoStatus =
  | 'ATIVO'
  | 'EXPIRADO'
  | 'FECHADO'
  | 'PERDIDO'
  | 'CANCELADO'
  | 'REPLICADO';

export const OrcamentoStatusEnum = {
  ATIVO: 'ATIVO',
  EXPIRADO: 'EXPIRADO',
  FECHADO: 'FECHADO',
  PERDIDO: 'PERDIDO',
  CANCELADO: 'CANCELADO',
  REPLICADO: 'REPLICADO',
} as const;

export interface OrcamentoListItem {
  idOrcamento: string;
  idLoja: number;
  idUsuario: number;
  idUsuarioConsultor: number | null;
  status: OrcamentoStatus;
  diasRestantes: string | number;
  vendedor: string;
  consultor: string | null;
  cliente: string;
  profissional: string | null;
  tel: string | null;
  telCel: string | null;
  telProf: string | null;
  data: Date;
  data2: string;
  total: number;
  dataFollowup: Date | null;
  dataProxFollowup: Date | null;
  observacao: string | null;
  semaforo: number | null;
  fornecedores: string | null;
}

export interface OrcamentoFilters {
  idLoja?: number;
  idVendedor?: number;
  fornecedor?: string;
  statuses?: OrcamentoStatus[];
  mesAno?: string;
  semaforo?: number;
  search?: string;
  apenasPropriosOrcamentos?: boolean;
}
