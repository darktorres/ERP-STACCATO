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
    /// Login with username and password
    /// </summary>
    [HttpPost("login")]
    public async Task<ActionResult<ApiResponse<LoginResponse>>> Login([FromBody] LoginRequest request)
    {
        _logger.LogInformation($"[Auth] Login attempt for user: {request.User}");

        var response = await _authService.LoginAsync(request);

        if (!response.Success)
        {
            _logger.LogWarning($"[Auth] Login failed for user: {request.User}");
            return Unauthorized(new ApiResponse<LoginResponse>
            {
                Success = false,
                Message = response.Message
            });
        }

        _logger.LogInformation($"[Auth] Login successful for user: {request.User}");
        return Ok(new ApiResponse<LoginResponse>
        {
            Success = true,
            Data = response
        });
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
            return Unauthorized(new ApiResponse<SessionUser>
            {
                Success = false,
                Message = "Invalid token"
            });
        }

        var user = await _authService.GetCurrentUserAsync(userId);

        if (user == null)
        {
            return NotFound(new ApiResponse<SessionUser>
            {
                Success = false,
                Message = "User not found"
            });
        }

        return Ok(new ApiResponse<SessionUser>
        {
            Success = true,
            Data = user
        });
    }
}
