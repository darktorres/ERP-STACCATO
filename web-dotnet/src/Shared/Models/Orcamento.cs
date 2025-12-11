namespace ERP.Staccato.Shared.Models;

/// <summary>
/// Budget (Orcamento) entity matching the C++ and TypeScript versions
/// </summary>
public class OrcamentoListItem
{
    public string IdOrcamento { get; set; } = string.Empty;
    public int IdLoja { get; set; }
    public int IdUsuario { get; set; }
    public int? IdUsuarioConsultor { get; set; }
    public string Status { get; set; } = string.Empty;
    public string DiasRestantes { get; set; } = string.Empty;
    public string Vendedor { get; set; } = string.Empty;
    public string? Consultor { get; set; }
    public string Cliente { get; set; } = string.Empty;
    public string? Profissional { get; set; }
    public string? Tel { get; set; }
    public string? TelCel { get; set; }
    public string? TelProf { get; set; }
    public DateTime Data { get; set; }
    public string Data2 { get; set; } = string.Empty; // YYYY-MM format
    public decimal Total { get; set; }
    public DateTime? DataFollowup { get; set; }
    public DateTime? DataProxFollowup { get; set; }
    public string? Observacao { get; set; }
    public int? Semaforo { get; set; } // 1=QUENTE, 2=MORNO, 3=FRIO
    public string? Fornecedores { get; set; }
}

/// <summary>
/// Filter criteria for orcamento list
/// </summary>
public class OrcamentoFilters
{
    public int? IdLoja { get; set; }
    public int? IdVendedor { get; set; }
    public string? Fornecedor { get; set; }
    public List<string> Statuses { get; set; } = new();
    public string? MesAno { get; set; } // YYYY-MM format
    public int? Semaforo { get; set; }
    public string? Search { get; set; }
    public bool ApenasPropriosOrcamentos { get; set; }
}

/// <summary>
/// Status enum matching C++/TypeScript
/// </summary>
public static class OrcamentoStatus
{
    public const string ATIVO = "ATIVO";
    public const string EXPIRADO = "EXPIRADO";
    public const string FECHADO = "FECHADO";
    public const string PERDIDO = "PERDIDO";
    public const string CANCELADO = "CANCELADO";
    public const string REPLICADO = "REPLICADO";

    public static List<string> AllStatuses => new()
    {
        ATIVO, EXPIRADO, FECHADO, PERDIDO, CANCELADO, REPLICADO
    };
}

/// <summary>
/// Loja (Store) filter dropdown item
/// </summary>
public class LojaDto
{
    public int IdLoja { get; set; }
    public string? Descricao { get; set; }
    public string? NomeFantasia { get; set; }
}

/// <summary>
/// Vendedor (Vendor) filter dropdown item
/// </summary>
public class VendedorDto
{
    public int IdUsuario { get; set; }
    public string Nome { get; set; } = string.Empty;
}

/// <summary>
/// Fornecedor (Supplier) filter dropdown item
/// </summary>
public class FornecedorDto
{
    public string RazaoSocial { get; set; } = string.Empty;
}
