using System.Net.Http.Json;
using ERP.Staccato.Shared.Models;
using Microsoft.AspNetCore.Components.Authorization;

namespace ERP.Staccato.Frontend.Services;

/// <summary>
/// Frontend authentication service
/// </summary>
public class AuthService
{
    private readonly HttpClient _httpClient;
    private readonly ILogger<AuthService> _logger;
    private readonly AuthenticationStateProvider _authStateProvider;
    private readonly CustomAuthenticationStateProvider _customAuthProvider;

    public AuthService(HttpClient httpClient, ILogger<AuthService> logger, AuthenticationStateProvider authStateProvider)
    {
        _httpClient = httpClient;
        _logger = logger;
        _authStateProvider = authStateProvider;
        _customAuthProvider = (CustomAuthenticationStateProvider)authStateProvider;
    }

    /// <summary>
    /// Login user with email and password
    /// </summary>
    public async Task<ApiResponse<LoginResponse>?> LoginAsync(LoginRequest request)
    {
        try
        {
            var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);

            if (!response.IsSuccessStatusCode)
            {
                _logger.LogWarning($"Login failed with status {response.StatusCode}");
                try
                {
                    var error = await response.Content.ReadAsAsync<ApiResponse<LoginResponse>>();
                    return error;
                }
                catch
                {
                    return new ApiResponse<LoginResponse> { Sucesso = false, Mensagem = "Login failed" };
                }
            }

            var result = await response.Content.ReadAsAsync<ApiResponse<LoginResponse>>();
            if (result == null)
            {
                return new ApiResponse<LoginResponse> { Sucesso = false, Mensagem = "Invalid response" };
            }

            if (result?.Sucesso == true && !string.IsNullOrEmpty(result.Dados?.Token))
            {
                await _customAuthProvider.MarkUserAsAuthenticatedAsync(result.Dados.Token);
                _logger.LogInformation("Login successful");
            }

            return result;
        }
        catch (Exception ex)
        {
            _logger.LogError($"Login error: {ex.Message}");
            return new ApiResponse<LoginResponse>
            {
                Sucesso = false,
                Mensagem = $"Erro ao fazer login: {ex.Message}"
            };
        }
    }

    /// <summary>
    /// Get current user
    /// </summary>
    public async Task<SessionUser?> GetCurrentUserAsync()
    {
        try
        {
            var response = await _httpClient.GetAsync("/api/auth/me");

            if (!response.IsSuccessStatusCode)
                return null;

            var result = await response.Content.ReadAsAsync<ApiResponse<SessionUser>>();
            return result?.Dados;
        }
        catch (Exception ex)
        {
            _logger.LogError($"GetCurrentUser error: {ex.Message}");
            return null;
        }
    }

    /// <summary>
    /// Get auth state
    /// </summary>
    public async Task<AuthenticationState?> GetAuthStateAsync()
    {
        try
        {
            return await _authStateProvider.GetAuthenticationStateAsync();
        }
        catch (Exception ex)
        {
            _logger.LogError($"GetAuthState error: {ex.Message}");
            return null;
        }
    }

    /// <summary>
    /// Logout user
    /// </summary>
    public async Task LogoutAsync()
    {
        try
        {
            await _customAuthProvider.MarkUserAsLoggedOutAsync();
            _logger.LogInformation("User logged out");
        }
        catch (Exception ex)
        {
            _logger.LogError($"Logout error: {ex.Message}");
        }
    }
}
