namespace ERP.Staccato.Shared.Models;

/// <summary>
/// Login request
/// </summary>
public class LoginRequest
{
    public string Email { get; set; } = string.Empty;
    public string Senha { get; set; } = string.Empty;
}

/// <summary>
/// Login response
/// </summary>
public class LoginResponse
{
    public bool Sucesso { get; set; }
    public string? Mensagem { get; set; }
    public string? Token { get; set; }
    public SessionUser? Usuario { get; set; }
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
