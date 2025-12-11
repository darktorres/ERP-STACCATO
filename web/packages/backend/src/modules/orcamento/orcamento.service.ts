import { Injectable } from '@nestjs/common';
import { PrismaService } from '../../prisma/prisma.service.js';
import { OrcamentoFilters } from '@erp-staccato/shared';

@Injectable()
export class OrcamentoService {
  constructor(private prisma: PrismaService) {}

  async list(filters: OrcamentoFilters, userId: number, userType: string, userLojaId: number, userName: string) {
    const startTime = Date.now();
    const times: Record<string, number> = {};

    try {
      times.start = Date.now();

      // Build WHERE clause based on filters (matching C++ widgetorcamento.cpp montaFiltro logic)
      const whereConditions: string[] = [];
      const params: any[] = [];

      // Store filter (gerentes see only their store, admin sees all)
      // C++ line 292: idLoja = <value>
      if (userType === 'GERENTE LOJA' || userType === 'GERENTE DEPARTAMENTO') {
        whereConditions.push('idLoja = ?');
        params.push(userLojaId);
      } else if (filters.idLoja) {
        whereConditions.push('idLoja = ?');
        params.push(filters.idLoja);
      }

      // Month filter (YYYY-MM format)
      // C++ line 298: data2 = 'yyyy-MM' (when checkbox is checked)
      if (filters.mesAno) {
        whereConditions.push('data2 = ?');
        params.push(filters.mesAno);
      }

      // Vendor filter (by ID)
      // C++ line 305: (idUsuario = <id> OR idUsuarioConsultor = <id>)
      if (filters.idVendedor) {
        whereConditions.push('(idUsuario = ? OR idUsuarioConsultor = ?)');
        params.push(filters.idVendedor, filters.idVendedor);
      }

      // Supplier filter (comma-separated list)
      // C++ line 312: (fornecedores LIKE '%<value>%')
      if (filters.fornecedor) {
        whereConditions.push('fornecedores LIKE ?');
        params.push(`%${filters.fornecedor}%`);
      }

      // Status filter (checkbox list)
      // C++ lines 323-331: status IN ('STATUS1', 'STATUS2', ...)
      if (filters.statuses && filters.statuses.length > 0) {
        const placeholders = filters.statuses.map(() => '?').join(',');
        whereConditions.push(`status IN (${placeholders})`);
        params.push(...filters.statuses);
      }

      // Followup semaforo filter (dropdown index)
      // C++ line 335: (semaforo = <index>) where index is 1=QUENTE, 2=MORNO, 3=FRIO
      if (filters.semaforo) {
        whereConditions.push('semaforo = ?');
        params.push(filters.semaforo);
      }

      // Radio button filter: "Próprios" (show only user's budgets by name)
      // C++ line 317: (vendedor = '<user_name>' OR consultor = '<user_name>')
      // Only applies to VENDEDOR/VENDEDOR ESPECIAL when they select "Próprios"
      if ((userType === 'VENDEDOR' || userType === 'VENDEDOR ESPECIAL') && filters.apenasPropriosOrcamentos) {
        whereConditions.push('(vendedor = ? OR consultor = ?)');
        params.push(userName, userName);
      }

      // Search filter (across multiple fields)
      // C++ line 342: (idOrcamento LIKE '%<text>%' OR vendedor LIKE '%<text>%' OR cliente LIKE '%<text>%' OR profissional LIKE '%<text>%')
      if (filters.search) {
        const searchTerm = `%${filters.search}%`;
        whereConditions.push('(idOrcamento LIKE ? OR vendedor LIKE ? OR cliente LIKE ? OR profissional LIKE ?)');
        params.push(searchTerm, searchTerm, searchTerm, searchTerm);
      }

      const whereClause = whereConditions.length > 0 ? `WHERE ${whereConditions.join(' AND ')}` : '';
      times.queryBuilt = Date.now();

      // Debug logging
      console.log('[Orcamento.list] Filter inputs:', {
        filters,
        userType,
        userId,
        userLojaId,
        userName,
      });
      console.log('[Orcamento.list] Built WHERE clause:', {
        whereConditions,
        params,
        whereClause,
      });

      // Query the view_orcamento directly (same as C++ widget does)
      // The view already has all the joins and computed fields optimized
      const fullQuery = `
        SELECT
          idOrcamento,
          idLoja,
          idUsuario,
          idUsuarioConsultor,
          status,
          diasRestantes,
          vendedor,
          consultor,
          cliente,
          profissional,
          tel,
          telCel,
          telProf,
          data,
          data2,
          total,
          idFollowup,
          dataFollowup,
          dataProxFollowup,
          observacao,
          semaforo,
          fornecedores
        FROM view_orcamento
        ${whereClause}
        ORDER BY data DESC
      `;

      console.log('[Orcamento.list] Full query:', { fullQuery, params });

      const orcamentos = await this.prisma.$queryRawUnsafe(fullQuery, ...params);
      times.queryExecuted = Date.now();

      // Convert BigInt values to numbers (needed for JSON serialization)
      const orcamentosArray = Array.isArray(orcamentos) ? orcamentos : [];
      const normalized = orcamentosArray.map((item: any) => {
        const normalized: any = {};
        for (const [key, value] of Object.entries(item)) {
          if (typeof value === 'bigint') {
            normalized[key] = Number(value);
          } else {
            normalized[key] = value;
          }
        }
        return normalized;
      });
      times.normalized = Date.now();

      console.log('[Orcamento.list] Performance metrics:', {
        totalMs: times.normalized - times.start,
        queryBuildMs: times.queryBuilt - times.start,
        queryExecuteMs: times.queryExecuted - times.queryBuilt,
        normalizationMs: times.normalized - times.queryExecuted,
        rowCount: normalized.length,
      });

      return normalized;
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
