using Microsoft.EntityFrameworkCore;
using ERP.Staccato.Backend.Data;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Backend.Services;

/// <summary>
/// Orcamento (Budget) service - mirrors TypeScript orcamento.service.ts
/// </summary>
public class OrcamentoService
{
    private readonly ApplicationDbContext _context;

    public OrcamentoService(ApplicationDbContext context)
    {
        _context = context;
    }

    /// <summary>
    /// List orcamentos with filtering
    /// Mirrors C++ WidgetOrcamento::montaFiltro() logic
    /// </summary>
    public async Task<List<OrcamentoListItem>> ListAsync(
        OrcamentoFilters filters,
        int userId,
        string userType,
        int userLojaId,
        string userName)
    {
        try
        {
            // Build raw SQL query similar to TypeScript implementation
            var query = @"
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
WHERE 1=1";

            var parameters = new List<object>();
            var paramIndex = 0;

            // Role-based filtering
            if (userType == "GERENTE LOJA" || userType == "GERENTE DEPARTAMENTO")
            {
                query += $" AND idLoja = @p{paramIndex}";
                parameters.Add(userLojaId);
                paramIndex++;
            }
            else if (filters.IdLoja.HasValue)
            {
                query += $" AND idLoja = @p{paramIndex}";
                parameters.Add(filters.IdLoja);
                paramIndex++;
            }

            // Month filter
            if (!string.IsNullOrEmpty(filters.MesAno))
            {
                query += $" AND data2 = @p{paramIndex}";
                parameters.Add(filters.MesAno);
                paramIndex++;
            }

            // Vendor filter
            if (filters.IdVendedor.HasValue)
            {
                query += $" AND (idUsuario = @p{paramIndex} OR idUsuarioConsultor = @p{paramIndex + 1})";
                parameters.Add(filters.IdVendedor);
                parameters.Add(filters.IdVendedor);
                paramIndex += 2;
            }

            // Supplier filter
            if (!string.IsNullOrEmpty(filters.Fornecedor))
            {
                query += $" AND fornecedores LIKE @p{paramIndex}";
                parameters.Add($"%{filters.Fornecedor}%");
                paramIndex++;
            }

            // Status filter
            if (filters.Statuses.Any())
            {
                var statusPlaceholders = string.Join(",", filters.Statuses
                    .Select((_, i) => $"@p{paramIndex + i}"));
                query += $" AND status IN ({statusPlaceholders})";
                parameters.AddRange(filters.Statuses);
                paramIndex += filters.Statuses.Count;
            }

            // Semaforo filter
            if (filters.Semaforo.HasValue)
            {
                query += $" AND semaforo = @p{paramIndex}";
                parameters.Add(filters.Semaforo);
                paramIndex++;
            }

            // "Próprios" filter (only for vendedores)
            if ((userType == "VENDEDOR" || userType == "VENDEDOR ESPECIAL")
                && filters.ApenasPropriosOrcamentos)
            {
                query += $" AND (vendedor = @p{paramIndex} OR consultor = @p{paramIndex + 1})";
                parameters.Add(userName);
                parameters.Add(userName);
                paramIndex += 2;
            }

            // Search filter
            if (!string.IsNullOrEmpty(filters.Search))
            {
                var searchTerm = $"%{filters.Search}%";
                query += $" AND (idOrcamento LIKE @p{paramIndex} OR vendedor LIKE @p{paramIndex + 1} " +
                         $"OR cliente LIKE @p{paramIndex + 2} OR profissional LIKE @p{paramIndex + 3})";
                parameters.Add(searchTerm);
                parameters.Add(searchTerm);
                parameters.Add(searchTerm);
                parameters.Add(searchTerm);
                paramIndex += 4;
            }

            // Order by
            query += " ORDER BY data DESC";

            Console.WriteLine($"[Orcamento.ListAsync] Query: {query}");
            Console.WriteLine($"[Orcamento.ListAsync] Filters: {System.Text.Json.JsonSerializer.Serialize(filters)}");

            // Execute query and map results
            var result = new List<OrcamentoListItem>();

            // Using FromSqlRaw with proper parameterization
            var orcamentos = await _context.Set<OrcamentoView>()
                .FromSqlRaw(query, parameters.ToArray())
                .ToListAsync();

            // Map to DTO
            result = orcamentos.Select(o => new OrcamentoListItem
            {
                IdOrcamento = o.IdOrcamento,
                IdLoja = o.IdLoja,
                IdUsuario = o.IdUsuario,
                IdUsuarioConsultor = o.IdUsuarioConsultor,
                Status = o.Status,
                DiasRestantes = o.DiasRestantes?.ToString() ?? "-",
                Vendedor = o.Vendedor,
                Consultor = o.Consultor,
                Cliente = o.Cliente,
                Profissional = o.Profissional,
                Tel = o.Tel,
                TelCel = o.TelCel,
                TelProf = o.TelProf,
                Data = o.Data,
                Data2 = o.Data2,
                Total = o.Total,
                DataFollowup = o.DataFollowup,
                DataProxFollowup = o.DataProxFollowup,
                Observacao = o.Observacao,
                Semaforo = o.Semaforo,
                Fornecedores = o.Fornecedores
            }).ToList();

            Console.WriteLine($"[Orcamento.ListAsync] Returned {result.Count} rows");

            return result;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Error listing orcamentos: {ex}");
            throw;
        }
    }

    /// <summary>
    /// Get lojas for filter dropdown
    /// </summary>
    public async Task<List<LojaDto>> GetLojasForFilterAsync()
    {
        return await _context.Set<Loja>()
            .Where(l => !l.Desativado)
            .OrderBy(l => l.Descricao)
            .Select(l => new LojaDto
            {
                IdLoja = l.IdLoja,
                Descricao = l.Descricao,
                NomeFantasia = l.NomeFantasia
            })
            .ToListAsync();
    }

    /// <summary>
    /// Get vendedores for filter dropdown
    /// </summary>
    public async Task<List<VendedorDto>> GetVendedoresForFilterAsync(int? idLoja = null)
    {
        var query = _context.Set<Usuario>()
            .Where(u => (u.Tipo == "VENDEDOR" || u.Tipo == "VENDEDOR ESPECIAL") && !u.Desativado);

        if (idLoja.HasValue)
            query = query.Where(u => u.IdLoja == idLoja);

        return await query
            .OrderBy(u => u.Nome)
            .Select(u => new VendedorDto
            {
                IdUsuario = u.IdUsuario,
                Nome = u.Nome
            })
            .ToListAsync();
    }

    /// <summary>
    /// Get fornecedores for filter dropdown
    /// </summary>
    public async Task<List<FornecedorDto>> GetFornecedoresForFilterAsync()
    {
        var result = await _context.Database.SqlQueryRaw<dynamic>(
            @"SELECT DISTINCT fornecedores FROM orcamento
              WHERE fornecedores IS NOT NULL AND fornecedores != ''"
        ).ToListAsync();

        var fornecedoresSet = new HashSet<string>();

        foreach (var row in result)
        {
            if (row?.fornecedores != null)
            {
                var fornecedores = row.fornecedores.ToString()?.Split(',') ?? Array.Empty<string>();
                foreach (var f in fornecedores)
                {
                    fornecedoresSet.Add(f.Trim());
                }
            }
        }

        return fornecedoresSet
            .OrderBy(x => x)
            .Select(x => new FornecedorDto { RazaoSocial = x })
            .ToList();
    }
}

/// <summary>
/// OrcamentoView entity for FromSqlRaw queries
/// </summary>
public class OrcamentoView
{
    public string IdOrcamento { get; set; } = string.Empty;
    public int IdLoja { get; set; }
    public int IdUsuario { get; set; }
    public int? IdUsuarioConsultor { get; set; }
    public string Status { get; set; } = string.Empty;
    public int? DiasRestantes { get; set; }
    public string Vendedor { get; set; } = string.Empty;
    public string? Consultor { get; set; }
    public string Cliente { get; set; } = string.Empty;
    public string? Profissional { get; set; }
    public string? Tel { get; set; }
    public string? TelCel { get; set; }
    public string? TelProf { get; set; }
    public DateTime Data { get; set; }
    public string Data2 { get; set; } = string.Empty;
    public decimal Total { get; set; }
    public int? IdFollowup { get; set; }
    public DateTime? DataFollowup { get; set; }
    public DateTime? DataProxFollowup { get; set; }
    public string? Observacao { get; set; }
    public int? Semaforo { get; set; }
    public string? Fornecedores { get; set; }
}
