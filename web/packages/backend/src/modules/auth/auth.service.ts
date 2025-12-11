import { Injectable, UnauthorizedException } from '@nestjs/common';
import { JwtService } from '@nestjs/jwt';
import { PrismaService } from '../../prisma/prisma.service.js';
import type { LoginInput, AuthorizationInput } from '@erp-staccato/shared';
import type { SessionUser, UserType } from '@erp-staccato/shared';

@Injectable()
export class AuthService {
  constructor(
    private prisma: PrismaService,
    private jwtService: JwtService,
  ) {}

  /**
   * Login user with username and password
   * Matches the C++ User::login() function behavior
   */
  async login(input: LoginInput) {
    const { user, password } = input;

    // Check maintenance mode first
    const maintenance = await this.prisma.maintenance.findFirst({
      where: { id: 1 },
    });

    if (maintenance?.emManutencao) {
      throw new UnauthorizedException('Sistema em manutenção!');
    }

    // Query user with SHA_PASSWORD comparison (matching C++ implementation)
    // Note: SHA_PASSWORD is a MySQL function, we use raw query
    const usuarios = await this.prisma.$queryRaw<
      Array<{
        idUsuario: number;
        idLoja: number;
        nome: string;
        tipo: string;
        desativado: boolean;
      }>
    >`
      SELECT idUsuario, idLoja, nome, tipo, desativado
      FROM usuario
      WHERE user = ${user}
        AND password = SHA_PASSWORD(${password})
        AND desativado = FALSE
    `;

    if (usuarios.length === 0) {
      throw new UnauthorizedException('Login inválido!');
    }

    const usuario = usuarios[0];

    // Ensure both IDs are regular numbers, not BigInt
    const idUsuarioAsNumber = typeof usuario.idUsuario === 'bigint' ? Number(usuario.idUsuario) : usuario.idUsuario;
    const idLojaAsNumber = typeof usuario.idLoja === 'bigint' ? Number(usuario.idLoja) : usuario.idLoja;

    // Block OPERACIONAL users (matching C++ behavior)
    if (usuario.tipo === 'OPERACIONAL') {
      throw new UnauthorizedException('Operacional bloqueado!');
    }

    // Get loja info
    const loja = await this.prisma.loja.findUnique({
      where: { idLoja: idLojaAsNumber },
      select: {
        idLoja: true,
        descricao: true,
        nomeFantasia: true,
      },
    });

    // Create session payload
    const sessionUser: SessionUser = {
      idUsuario: idUsuarioAsNumber,
      idLoja: idLojaAsNumber,
      user: user,
      tipo: usuario.tipo as UserType,
      nome: usuario.nome,
    };

    // Generate JWT token
    const token = this.jwtService.sign(sessionUser);

    return {
      success: true,
      token,
      user: {
        ...sessionUser,
        loja,
      },
    };
  }

  /**
   * Authorization with one-time password
   * Matches the C++ User::autorizacao() function behavior
   */
  async authorize(input: AuthorizationInput) {
    const { user, senhaUsoUnico } = input;

    // Query for authorization (managers/admins only)
    const usuarios = await this.prisma.$queryRaw<
      Array<{
        idUsuario: number;
        valorMinimoFrete: number | null;
      }>
    >`
      SELECT idUsuario, valorMinimoFrete
      FROM usuario
      WHERE user = ${user}
        AND senhaUsoUnico = ${senhaUsoUnico}
        AND tipo IN ('ADMINISTRADOR', 'ADMINISTRATIVO', 'DIRETOR', 'GERENTE DEPARTAMENTO', 'GERENTE LOJA')
    `;

    if (usuarios.length === 0) {
      throw new UnauthorizedException('Senha não confere!');
    }

    const usuario = usuarios[0];

    // Clear one-time password after use
    await this.prisma.usuario.update({
      where: { idUsuario: usuario.idUsuario },
      data: {
        senhaUsoUnico: null,
        valorMinimoFrete: null,
      },
    });

    return {
      success: true,
      valorMinimoFrete: usuario.valorMinimoFrete,
    };
  }

  /**
   * Validate JWT token and return user
   */
  async validateToken(payload: SessionUser) {
    const usuario = await this.prisma.usuario.findUnique({
      where: { idUsuario: payload.idUsuario },
      select: {
        idUsuario: true,
        idLoja: true,
        user: true,
        tipo: true,
        nome: true,
        desativado: true,
      },
    });

    if (!usuario || usuario.desativado) {
      throw new UnauthorizedException('Usuário inválido ou desativado');
    }

    return usuario;
  }
}
