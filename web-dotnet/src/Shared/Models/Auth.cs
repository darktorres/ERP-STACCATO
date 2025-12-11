namespace ERP.Staccato.Shared.Models;

/// <summary>
/// Login request
/// </summary>
public class LoginRequest
{
    public string User { get; set; } = string.Empty;
    public string Password { get; set; } = string.Empty;
    public bool Staging { get; set; }
}

/// <summary>
/// Login response
/// </summary>
public class LoginResponse
{
    public bool Success { get; set; }
    public string? Message { get; set; }
    public string? Token { get; set; }
    public SessionUser? User { get; set; }
}

/// <summary>
/// API response wrapper
/// </summary>
public class ApiResponse<T>
{
    public bool Success { get; set; }
    public T? Data { get; set; }
    public string? Message { get; set; }
    public Dictionary<string, List<string>>? Errors { get; set; }

    public static ApiResponse<T> Ok(T data) => new() { Success = true, Data = data };
    public static ApiResponse<T> Error(string message) => new() { Success = false, Message = message };
    public static ApiResponse<T> ValidationError(Dictionary<string, List<string>> errors) =>
        new() { Success = false, Errors = errors, Message = "Validation error" };
}
