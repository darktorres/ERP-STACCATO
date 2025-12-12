namespace ERP.Staccato.Shared.Models;

/// <summary>
/// Represents the authenticated user session
/// Matches TypeScript SessionUser interface
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
    /// User role types - matches TypeScript UserType enum
    /// </summary>
    public static class Roles
    {
        public const string ADMINISTRADOR = "ADMINISTRADOR";
        public const string ADMINISTRATIVO = "ADMINISTRATIVO";
        public const string ASSISTENTE_ADMINISTRATIVO = "ASSISTENTE ADMINISTRATIVO";
        public const string DIRETOR = "DIRETOR";
        public const string GERENTE_DEPARTAMENTO = "GERENTE DEPARTAMENTO";
        public const string GERENTE_FINANCEIRO = "GERENTE FINANCEIRO";
        public const string GERENTE_LOJA = "GERENTE LOJA";
        public const string OPERACIONAL = "OPERACIONAL";
        public const string VENDEDOR = "VENDEDOR";
        public const string VENDEDOR_ESPECIAL = "VENDEDOR ESPECIAL";
    }
}

/// <summary>
/// Role helper methods - matches TypeScript userTypeHelpers
/// </summary>
public static class RoleHelpers
{
    public static bool IsAdmin(string tipo) =>
        tipo == SessionUser.Roles.ADMINISTRADOR || tipo == SessionUser.Roles.DIRETOR;

    public static bool IsAdministrativo(string tipo) =>
        tipo == SessionUser.Roles.ADMINISTRADOR ||
        tipo == SessionUser.Roles.ADMINISTRATIVO ||
        tipo == SessionUser.Roles.DIRETOR;

    public static bool IsGerente(string tipo) =>
        tipo == SessionUser.Roles.GERENTE_DEPARTAMENTO ||
        tipo == SessionUser.Roles.GERENTE_FINANCEIRO ||
        tipo == SessionUser.Roles.GERENTE_LOJA;

    public static bool IsVendedor(string tipo) =>
        tipo == SessionUser.Roles.VENDEDOR;

    public static bool IsVendedorOrEspecial(string tipo) =>
        tipo == SessionUser.Roles.VENDEDOR ||
        tipo == SessionUser.Roles.VENDEDOR_ESPECIAL;

    public static bool IsOperacional(string tipo) =>
        tipo == SessionUser.Roles.OPERACIONAL;

    public static bool IsAssistenteAdministrativo(string tipo) =>
        tipo == SessionUser.Roles.ASSISTENTE_ADMINISTRATIVO;

    public static bool CanAuthorize(string tipo) =>
        IsAdmin(tipo) ||
        tipo == SessionUser.Roles.ADMINISTRATIVO ||
        IsGerente(tipo);
}
