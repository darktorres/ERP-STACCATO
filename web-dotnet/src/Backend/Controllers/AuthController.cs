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
    /// Login with user and password
    /// </summary>
    [HttpPost("login")]
    public async Task<ActionResult<LoginResponse>> Login([FromBody] LoginRequest request)
    {
        _logger.LogInformation($"[Auth] Login attempt for user: {request.User}");

        var response = await _authService.LoginAsync(request);

        if (!response.Success)
        {
            _logger.LogWarning($"[Auth] Login failed for user: {request.User} - {response.Error}");
            return Unauthorized(response);
        }

        _logger.LogInformation($"[Auth] Login successful for user: {request.User}");
        return Ok(response);
    }

    /// <summary>
    /// Authorization with one-time password
    /// Matches TypeScript tRPC router authorize procedure
    /// </summary>
    [HttpPost("authorize")]
    public async Task<ActionResult<AuthorizationResponse>> Authorize([FromBody] AuthorizationRequest request)
    {
        _logger.LogInformation($"[Auth] Authorization attempt for user: {request.User}");

        var response = await _authService.AuthorizeAsync(request);

        if (!response.Success)
        {
            _logger.LogWarning($"[Auth] Authorization failed for user: {request.User}");
            return Unauthorized(response);
        }

        _logger.LogInformation($"[Auth] Authorization successful for user: {request.User}");
        return Ok(response);
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
