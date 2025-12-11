import type { UserType } from './enums.js';

// Matches usuario table structure from initdb.sql
export interface User {
  idUsuario: number;
  idLoja: number;
  user: string;
  tipo: UserType;
  nome: string;
  email: string | null;
  telefone: string | null;
  desativado: boolean;
}

// User with related loja data (for login response)
export interface UserWithLoja extends User {
  loja: {
    idLoja: number;
    descricao: string | null;
    nomeFantasia: string;
  };
}

// Session data stored in JWT/cookie
export interface SessionUser {
  idUsuario: number;
  idLoja: number;
  user: string;
  tipo: UserType;
  nome: string;
}
