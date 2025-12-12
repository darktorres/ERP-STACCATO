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
    /// Login user with user and password
    /// </summary>
    public async Task<LoginResponse?> LoginAsync(string user, string password)
    {
        try
        {
            var request = new LoginRequest
            {
                User = user,
                Password = password,
                Staging = false
            };

            var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);

            if (!response.IsSuccessStatusCode)
            {
                _logger.LogWarning($"Login failed with status {response.StatusCode}");
                try
                {
                    var error = await response.Content.ReadAsAsync<LoginResponse>();
                    return error ?? new LoginResponse { Success = false, Error = "Login failed" };
                }
                catch
                {
                    return new LoginResponse { Success = false, Error = "Login failed" };
                }
            }

            var result = await response.Content.ReadAsAsync<LoginResponse>();
            if (result == null)
            {
                return new LoginResponse { Success = false, Error = "Invalid response" };
            }

            if (result?.Success == true && !string.IsNullOrEmpty(result.Token))
            {
                await _customAuthProvider.MarkUserAsAuthenticatedAsync(result.Token);
                _logger.LogInformation("Login successful");
            }

            return result;
        }
        catch (Exception ex)
        {
            _logger.LogError($"Login error: {ex.Message}");
            return new LoginResponse
            {
                Success = false,
                Error = $"Erro ao fazer login: {ex.Message}"
            };
        }
    }

    /// <summary>
    /// Authorization with one-time password
    /// </summary>
    public async Task<AuthorizationResponse?> AuthorizeAsync(string user, string senhaUsoUnico)
    {
        try
        {
            var request = new AuthorizationRequest
            {
                User = user,
                SenhaUsoUnico = senhaUsoUnico
            };

            var response = await _httpClient.PostAsJsonAsync("/api/auth/authorize", request);

            var result = await response.Content.ReadAsAsync<AuthorizationResponse>();
            return result ?? new AuthorizationResponse { Success = false, Message = "Invalid response" };
        }
        catch (Exception ex)
        {
            _logger.LogError($"Authorization error: {ex.Message}");
            return new AuthorizationResponse
            {
                Success = false,
                Message = $"Erro na autorização: {ex.Message}"
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
