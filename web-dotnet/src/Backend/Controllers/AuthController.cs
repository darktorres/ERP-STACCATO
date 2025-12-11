using Microsoft.AspNetCore.Mvc;
using ERP.Staccato.Backend.Services;
using ERP.Staccato.Shared.Models;
using System.Security.Claims;

namespace ERP.Staccato.Backend.Controllers;

[ApiController]
[Route("api/[controller]")]
public class AuthController : ControllerBase
{
    private readonly AuthService _authService;
    private readonly ILogger<AuthController> _logger;

    public AuthController(AuthService authService, ILogger<AuthController> logger)
    {
        _authService = authService;
        _logger = logger;
    }

    /// <summary>
    /// Login with email and password
    /// </summary>
    [HttpPost("login")]
    public async Task<ActionResult<ApiResponse<LoginResponse>>> Login([FromBody] LoginRequest request)
    {
        _logger.LogInformation($"[Auth] Login attempt for email: {request.Email}");

        var response = await _authService.LoginAsync(request);

        if (!response.Sucesso)
        {
            _logger.LogWarning($"[Auth] Login failed for email: {request.Email}");
            return Unauthorized(ApiResponse<LoginResponse>.Error(response.Mensagem ?? "Login failed"));
        }

        _logger.LogInformation($"[Auth] Login successful for email: {request.Email}");
        return Ok(ApiResponse<LoginResponse>.Ok(response));
    }

    /// <summary>
    /// Get current authenticated user
    /// </summary>
    [HttpGet("me")]
    public async Task<ActionResult<ApiResponse<SessionUser>>> GetMe()
    {
        var userIdClaim = User.FindFirst(ClaimTypes.NameIdentifier)?.Value;

        if (!int.TryParse(userIdClaim, out var userId))
        {
            return Unauthorized(ApiResponse<SessionUser>.Error("Invalid token"));
        }

        var user = await _authService.GetCurrentUserAsync(userId);

        if (user == null)
        {
            return NotFound(ApiResponse<SessionUser>.Error("User not found"));
        }

        return Ok(ApiResponse<SessionUser>.Ok(user));
    }
}
