// User types from the original C++ codebase
// Maps to: usuario.tipo in MySQL
export const UserType = {
  ADMINISTRADOR: 'ADMINISTRADOR',
  ADMINISTRATIVO: 'ADMINISTRATIVO',
  ASSISTENTE_ADMINISTRATIVO: 'ASSISTENTE ADMINISTRATIVO',
  DIRETOR: 'DIRETOR',
  GERENTE_DEPARTAMENTO: 'GERENTE DEPARTAMENTO',
  GERENTE_FINANCEIRO: 'GERENTE FINANCEIRO',
  GERENTE_LOJA: 'GERENTE LOJA',
  OPERACIONAL: 'OPERACIONAL',
  VENDEDOR: 'VENDEDOR',
  VENDEDOR_ESPECIAL: 'VENDEDOR ESPECIAL',
} as const;

export type UserType = (typeof UserType)[keyof typeof UserType];

// Helper functions matching C++ User::isAdmin(), etc.
export const userTypeHelpers = {
  isAdmin: (tipo: UserType) =>
    tipo === UserType.ADMINISTRADOR || tipo === UserType.DIRETOR,

  isAdministrativo: (tipo: UserType) =>
    tipo === UserType.ADMINISTRADOR ||
    tipo === UserType.ADMINISTRATIVO ||
    tipo === UserType.DIRETOR,

  isEspecial: (tipo: UserType) => tipo === UserType.VENDEDOR_ESPECIAL,

  isGerente: (tipo: UserType) =>
    tipo === UserType.GERENTE_DEPARTAMENTO ||
    tipo === UserType.GERENTE_FINANCEIRO ||
    tipo === UserType.GERENTE_LOJA,

  isOperacional: (tipo: UserType) => tipo === UserType.OPERACIONAL,

  isAssistenteAdministrativo: (tipo: UserType) =>
    tipo === UserType.ASSISTENTE_ADMINISTRATIVO,

  isVendedor: (tipo: UserType) => tipo === UserType.VENDEDOR,

  isVendedorOrEspecial: (tipo: UserType) =>
    tipo === UserType.VENDEDOR || tipo === UserType.VENDEDOR_ESPECIAL,

  // Users that can authorize operations
  canAuthorize: (tipo: UserType) =>
    tipo === UserType.ADMINISTRADOR ||
    tipo === UserType.ADMINISTRATIVO ||
    tipo === UserType.DIRETOR ||
    tipo === UserType.GERENTE_DEPARTAMENTO ||
    tipo === UserType.GERENTE_LOJA,
};
