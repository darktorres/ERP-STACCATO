using System.Net.Http.Json;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Frontend.Services;

/// <summary>
/// Frontend authentication service
/// </summary>
public class AuthService
{
    private readonly HttpClient _httpClient;
    private readonly ILogger<AuthService> _logger;

    public AuthService(HttpClient httpClient, ILogger<AuthService> logger)
    {
        _httpClient = httpClient;
        _logger = logger;
    }

    /// <summary>
    /// Login user
    /// </summary>
    public async Task<LoginResponse?> LoginAsync(string user, string password, bool staging = false)
    {
        try
        {
            var request = new LoginRequest
            {
                User = user,
                Password = password,
                Staging = staging
            };

            var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);

            if (!response.IsSuccessStatusCode)
            {
                _logger.LogWarning($"Login failed with status {response.StatusCode}");
                return null;
            }

            var result = await response.Content.ReadAsAsync<ApiResponse<LoginResponse>>();
            return result?.Data;
        }
        catch (Exception ex)
        {
            _logger.LogError($"Login error: {ex.Message}");
            return null;
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
            return result?.Data;
        }
        catch (Exception ex)
        {
            _logger.LogError($"GetCurrentUser error: {ex.Message}");
            return null;
        }
    }
}
