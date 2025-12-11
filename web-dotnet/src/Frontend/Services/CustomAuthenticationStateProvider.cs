using System.Security.Claims;
using System.Text.Json;
using System.IdentityModel.Tokens.Jwt;
using Microsoft.AspNetCore.Components.Authorization;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Frontend.Services;

/// <summary>
/// Custom authentication state provider for JWT authentication
/// </summary>
public class CustomAuthenticationStateProvider : AuthenticationStateProvider
{
    private readonly ILocalStorageService _localStorage;
    private readonly ILogger<CustomAuthenticationStateProvider> _logger;

    public CustomAuthenticationStateProvider(ILocalStorageService localStorage, ILogger<CustomAuthenticationStateProvider> logger)
    {
        _localStorage = localStorage;
        _logger = logger;
    }

    /// <summary>
    /// Get authentication state from token
    /// </summary>
    public override async Task<AuthenticationState> GetAuthenticationStateAsync()
    {
        try
        {
            var token = await _localStorage.GetItemAsync("auth-token");

            if (string.IsNullOrEmpty(token))
            {
                return new AuthenticationState(new ClaimsPrincipal());
            }

            var claims = ParseClaimsFromJwt(token);
            var principal = new ClaimsPrincipal(new ClaimsIdentity(claims, "jwt"));

            return new AuthenticationState(principal);
        }
        catch (Exception ex)
        {
            _logger.LogError($"Authentication state error: {ex.Message}");
            return new AuthenticationState(new ClaimsPrincipal());
        }
    }

    /// <summary>
    /// Mark user as authenticated
    /// </summary>
    public async Task MarkUserAsAuthenticatedAsync(string token)
    {
        await _localStorage.SetItemAsync("auth-token", token);
        var claims = ParseClaimsFromJwt(token);
        var principal = new ClaimsPrincipal(new ClaimsIdentity(claims, "jwt"));
        NotifyAuthenticationStateChanged(Task.FromResult(new AuthenticationState(principal)));
    }

    /// <summary>
    /// Mark user as logged out
    /// </summary>
    public async Task MarkUserAsLoggedOutAsync()
    {
        await _localStorage.RemoveItemAsync("auth-token");
        NotifyAuthenticationStateChanged(Task.FromResult(new AuthenticationState(new ClaimsPrincipal())));
    }

    /// <summary>
    /// Parse claims from JWT token
    /// </summary>
    private List<Claim> ParseClaimsFromJwt(string jwt)
    {
        var claims = new List<Claim>();

        try
        {
            var handler = new JwtSecurityTokenHandler();
            var token = handler.ReadToken(jwt) as JwtSecurityToken;

            if (token == null)
                return claims;

            claims = token.Claims.ToList();
        }
        catch (Exception ex)
        {
            _logger.LogError($"JWT parsing error: {ex.Message}");
        }

        return claims;
    }
}

/// <summary>
/// Local storage service interface
/// </summary>
public interface ILocalStorageService
{
    Task<string?> GetItemAsync(string key);
    Task SetItemAsync(string key, string value);
    Task RemoveItemAsync(string key);
}

/// <summary>
/// Local storage implementation using Blazor's local storage
/// </summary>
public class LocalStorageService : ILocalStorageService
{
    // Note: In a real Blazor app, use Microsoft.AspNetCore.Components.Web.Extensions.Storage
    // For now, this is a placeholder

    public Task<string?> GetItemAsync(string key)
    {
        // This would be implemented using JS interop in real app
        return Task.FromResult<string?>(null);
    }

    public Task SetItemAsync(string key, string value)
    {
        // This would be implemented using JS interop in real app
        return Task.CompletedTask;
    }

    public Task RemoveItemAsync(string key)
    {
        // This would be implemented using JS interop in real app
        return Task.CompletedTask;
    }
}
