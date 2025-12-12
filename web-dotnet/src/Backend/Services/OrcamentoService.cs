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
    /// Implemented using LINQ for compatibility with both SQL Server and in-memory testing
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
            var query = _context.Set<OrcamentoView>().AsQueryable();

            // Role-based filtering
            if (userType == "GERENTE LOJA" || userType == "GERENTE DEPARTAMENTO")
            {
                query = query.Where(o => o.IdLoja == userLojaId);
            }
            else if (filters.IdLoja.HasValue)
            {
                query = query.Where(o => o.IdLoja == filters.IdLoja);
            }

            // Month filter
            if (!string.IsNullOrEmpty(filters.MesAno))
            {
                query = query.Where(o => o.Data2 == filters.MesAno);
            }

            // Vendor filter
            if (filters.IdVendedor.HasValue)
            {
                query = query.Where(o =>
                    o.IdUsuario == filters.IdVendedor ||
                    o.IdUsuarioConsultor == filters.IdVendedor);
            }

            // Supplier filter
            if (!string.IsNullOrEmpty(filters.Fornecedor))
            {
                query = query.Where(o =>
                    o.Fornecedores != null &&
                    o.Fornecedores.Contains(filters.Fornecedor));
            }

            // Status filter
            if (filters.Statuses.Any())
            {
                query = query.Where(o => filters.Statuses.Contains(o.Status));
            }

            // Semaforo filter
            if (filters.Semaforo.HasValue)
            {
                query = query.Where(o => o.Semaforo == filters.Semaforo);
            }

            // "Próprios" filter (only for vendedores)
            if ((userType == "VENDEDOR" || userType == "VENDEDOR ESPECIAL")
                && filters.ApenasPropriosOrcamentos)
            {
                query = query.Where(o =>
                    o.Vendedor == userName ||
                    o.Consultor == userName);
            }

            // Search filter
            if (!string.IsNullOrEmpty(filters.Search))
            {
                var searchTerm = filters.Search;
                query = query.Where(o =>
                    o.IdOrcamento.ToString().Contains(searchTerm) ||
                    (o.Vendedor != null && o.Vendedor.Contains(searchTerm)) ||
                    (o.Cliente != null && o.Cliente.Contains(searchTerm)) ||
                    (o.Profissional != null && o.Profissional.Contains(searchTerm))
                );
            }

            // Order by
            query = query.OrderByDescending(o => o.Data);

            Console.WriteLine($"[Orcamento.ListAsync] Filters: {System.Text.Json.JsonSerializer.Serialize(filters)}");

            // Execute query and map results
            var orcamentos = await query.ToListAsync();

            var result = orcamentos.Select(o => new OrcamentoListItem
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
            .OrderBy(l => l.IdLoja)
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
        // Get all orcamentos with fornecedores data
        var orcamentos = await _context.Set<OrcamentoView>()
            .Where(o => o.Fornecedores != null && o.Fornecedores != "")
            .Select(o => o.Fornecedores)
            .ToListAsync();

        var fornecedoresSet = new HashSet<string>();

        foreach (var fornecedoresStr in orcamentos)
        {
            if (!string.IsNullOrEmpty(fornecedoresStr))
            {
                var fornecedores = fornecedoresStr.Split(',');
                foreach (var f in fornecedores)
                {
                    var trimmed = f.Trim();
                    if (!string.IsNullOrEmpty(trimmed))
                        fornecedoresSet.Add(trimmed);
                }
            }
        }

        return fornecedoresSet
            .OrderBy(x => x)
            .Select(x => new FornecedorDto { RazaoSocial = x })
            .ToList();
    }
}
