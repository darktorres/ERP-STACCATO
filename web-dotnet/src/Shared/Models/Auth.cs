namespace ERP.Staccato.Shared.Models;

/// <summary>
/// Login request - matches TypeScript LoginInput
/// </summary>
public class LoginRequest
{
    public string User { get; set; } = string.Empty;
    public string Password { get; set; } = string.Empty;
    public bool Staging { get; set; } = false;
}

/// <summary>
/// Authorization request - one-time password validation
/// Matches TypeScript AuthorizationInput
/// </summary>
public class AuthorizationRequest
{
    public string User { get; set; } = string.Empty;
    public string SenhaUsoUnico { get; set; } = string.Empty;
}

/// <summary>
/// Authorization response
/// </summary>
public class AuthorizationResponse
{
    public bool Success { get; set; }
    public decimal? ValorMinimoFrete { get; set; }
    public string? Message { get; set; }
}

/// <summary>
/// Store information for login response
/// </summary>
public class LojaInfo
{
    public int IdLoja { get; set; }
    public string Descricao { get; set; } = string.Empty;
    public string NomeFantasia { get; set; } = string.Empty;
}

/// <summary>
/// Session user with loja information
/// </summary>
public class SessionUserWithLoja : SessionUser
{
    public LojaInfo? Loja { get; set; }
}

/// <summary>
/// Login response - matches TypeScript LoginResponse
/// </summary>
public class LoginResponse
{
    public bool Success { get; set; }
    public string? Token { get; set; }
    public SessionUserWithLoja? User { get; set; }
    public string? Error { get; set; }
}

/// <summary>
/// API response wrapper
/// </summary>
public class ApiResponse<T>
{
    public bool Sucesso { get; set; }
    public T? Dados { get; set; }
    public string? Mensagem { get; set; }
    public Dictionary<string, List<string>>? Erros { get; set; }

    public static ApiResponse<T> Ok(T data) => new() { Sucesso = true, Dados = data };
    public static ApiResponse<T> Error(string message) => new() { Sucesso = false, Mensagem = message };
    public static ApiResponse<T> ValidationError(Dictionary<string, List<string>> errors) =>
        new() { Sucesso = false, Erros = errors, Mensagem = "Validation error" };
}
