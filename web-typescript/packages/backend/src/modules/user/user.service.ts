import { Injectable } from '@nestjs/common';
import { PrismaService } from '../../prisma/prisma.service.js';

@Injectable()
export class UserService {
  constructor(private prisma: PrismaService) {}

  async findById(idUsuario: number) {
    return this.prisma.usuario.findUnique({
      where: { idUsuario },
      include: {
        loja: {
          select: {
            idLoja: true,
            descricao: true,
            nomeFantasia: true,
          },
        },
      },
    });
  }

  async findByUsername(user: string) {
    return this.prisma.usuario.findUnique({
      where: { user },
      include: {
        loja: {
          select: {
            idLoja: true,
            descricao: true,
            nomeFantasia: true,
          },
        },
      },
    });
  }

  /**
   * Get user setting from loja (matches C++ User::fromLoja)
   */
  async fromLoja(parameter: string, idUsuario: number) {
    const usuario = await this.prisma.usuario.findUnique({
      where: { idUsuario },
      include: { loja: true },
    });

    if (!usuario?.loja) {
      throw new Error(`Dados da loja/usuário não encontrados para o usuário: '${idUsuario}'`);
    }

    return (usuario.loja as Record<string, unknown>)[parameter];
  }

  /**
   * Check if user has specific permission
   */
  async temPermissao(idUsuario: number, permissao: string): Promise<boolean> {
    const result = await this.prisma.$queryRawUnsafe<Array<Record<string, boolean>>>(
      `SELECT ${permissao} FROM usuario_has_permissao WHERE idUsuario = ?`,
      idUsuario,
    );

    if (result.length === 0) {
      throw new Error(`Permissões não encontradas para usuário com id: '${idUsuario}'`);
    }

    return result[0][permissao] ?? false;
  }
}
