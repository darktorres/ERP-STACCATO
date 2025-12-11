namespace ERP.Staccato.Shared.Models;

/// <summary>
/// Represents the authenticated user session
/// </summary>
public class SessionUser
{
    public int IdUsuario { get; set; }
    public string User { get; set; } = string.Empty;
    public string Nome { get; set; } = string.Empty;
    public string Tipo { get; set; } = string.Empty;
    public int IdLoja { get; set; }
    public string? Email { get; set; }

    /// <summary>
    /// User role types
    /// </summary>
    public static class Roles
    {
        public const string ADMINISTRADOR = "ADMINISTRADOR";
        public const string ADMINISTRATIVO = "ADMINISTRATIVO";
        public const string GERENTE_LOJA = "GERENTE LOJA";
        public const string GERENTE_DEPARTAMENTO = "GERENTE DEPARTAMENTO";
        public const string VENDEDOR = "VENDEDOR";
        public const string VENDEDOR_ESPECIAL = "VENDEDOR ESPECIAL";
    }
}
