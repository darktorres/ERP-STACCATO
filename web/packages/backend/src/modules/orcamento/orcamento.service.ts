import { Injectable } from '@nestjs/common';
import { PrismaService } from '../../prisma/prisma.service.js';
import { OrcamentoFilters } from '@erp-staccato/shared';

@Injectable()
export class OrcamentoService {
  constructor(private prisma: PrismaService) {}

  async list(filters: OrcamentoFilters, userId: number, userType: string, userLojaId: number) {
    try {
      // Build WHERE clause based on filters
      const whereConditions: string[] = [];
      const params: any[] = [];

      // Role-based filtering for vendedor
      if (userType === 'VENDEDOR' || userType === 'VENDEDOR ESPECIAL') {
        if (filters.apenasPropriosOrcamentos) {
          whereConditions.push('(o.idUsuario = ? OR o.idUsuarioConsultor = ?)');
          params.push(userId, userId);
        }
      }

      // Store filter (gerentes see only their store, admin sees all)
      if (userType === 'GERENTE LOJA' || userType === 'GERENTE DEPARTAMENTO') {
        whereConditions.push('o.idLoja = ?');
        params.push(userLojaId);
      } else if (filters.idLoja) {
        whereConditions.push('o.idLoja = ?');
        params.push(filters.idLoja);
      }

      // Status filter
      if (filters.statuses && filters.statuses.length > 0) {
        const placeholders = filters.statuses.map(() => '?').join(',');
        whereConditions.push(`o.status IN (${placeholders})`);
        params.push(...filters.statuses);
      }

      // Month filter (YYYY-MM format)
      if (filters.mesAno) {
        whereConditions.push('o.data2 = ?');
        params.push(filters.mesAno);
      }

      // Vendor filter
      if (filters.idVendedor) {
        whereConditions.push('(o.idUsuario = ? OR o.idUsuarioConsultor = ?)');
        params.push(filters.idVendedor, filters.idVendedor);
      }

      // Supplier filter
      if (filters.fornecedor) {
        whereConditions.push('FIND_IN_SET(?, o.fornecedores)');
        params.push(filters.fornecedor);
      }

      // Search filter (idOrcamento, cliente, profissional, vendedor)
      if (filters.search) {
        const searchTerm = `%${filters.search}%`;
        whereConditions.push('(o.idOrcamento LIKE ? OR c.nome_razao LIKE ? OR p.nome_razao LIKE ? OR u.nome LIKE ?)');
        params.push(searchTerm, searchTerm, searchTerm, searchTerm);
      }

      const whereClause = whereConditions.length > 0 ? `WHERE ${whereConditions.join(' AND ')}` : '';

      // Use raw query to access view_orcamento which has computed fields
      const orcamentos = await this.prisma.$queryRawUnsafe(
        `
        SELECT
          o.idOrcamento,
          o.idLoja,
          o.idUsuario,
          o.idUsuarioConsultor,
          o.status,
          CASE
            WHEN o.status = 'FECHADO' THEN ''
            WHEN o.status = 'PERDIDO' THEN ''
            WHEN o.status = 'CANCELADO' THEN ''
            WHEN DATEDIFF(DATE_ADD(o.data, INTERVAL o.validade DAY), CURDATE()) < 0 THEN 'EXPIRADO'
            ELSE DATEDIFF(DATE_ADD(o.data, INTERVAL o.validade DAY), CURDATE())
          END as diasRestantes,
          u.nome as vendedor,
          uc.nome as consultor,
          c.nome_razao as cliente,
          p.nome_razao as profissional,
          c.tel,
          c.telCel,
          p.tel as telProf,
          o.data,
          o.data2,
          o.total,
          o.created,
          o.lastUpdated,
          o.observacao,
          o.fornecedores
        FROM orcamento o
        LEFT JOIN usuario u ON o.idUsuario = u.idUsuario
        LEFT JOIN usuario uc ON o.idUsuarioConsultor = uc.idUsuario
        LEFT JOIN cliente c ON o.idCliente = c.idCliente
        LEFT JOIN profissional p ON o.idProfissional = p.idProfissional
        ${whereClause}
        ORDER BY o.data DESC
        `,
        ...params
      );

      return orcamentos || [];
    } catch (error) {
      console.error('Error listing orcamentos:', error);
      throw error;
    }
  }

  async getLojasForFilter() {
    return this.prisma.loja.findMany({
      where: { desativado: false },
      select: { idLoja: true, descricao: true, nomeFantasia: true },
      orderBy: { descricao: 'asc' },
    });
  }

  async getVendedoresForFilter(idLoja?: number) {
    return this.prisma.usuario.findMany({
      where: {
        tipo: { in: ['VENDEDOR', 'VENDEDOR ESPECIAL'] },
        desativado: false,
        ...(idLoja && { idLoja }),
      },
      select: { idUsuario: true, nome: true },
      orderBy: { nome: 'asc' },
    });
  }

  async getFornecedoresForFilter() {
    // Extract unique fornecedores from orcamentos
    const result = await this.prisma.$queryRaw<{ fornecedores: string | null }[]>`
      SELECT DISTINCT fornecedores FROM orcamento
      WHERE fornecedores IS NOT NULL AND fornecedores != ''
    `;

    const fornecedoresSet = new Set<string>();
    result.forEach((row: { fornecedores: string | null }) => {
      if (row.fornecedores) {
        row.fornecedores.split(',').forEach((f: string) => {
          fornecedoresSet.add(f.trim());
        });
      }
    });

    return Array.from(fornecedoresSet)
      .sort()
      .map((name) => ({ razaoSocial: name }));
  }
}
