using System.IdentityModel.Tokens.Jwt;
using System.Security.Claims;
using System.Text;
using Microsoft.IdentityModel.Tokens;
using ERP.Staccato.Backend.Data;
using ERP.Staccato.Shared.Models;
using Microsoft.EntityFrameworkCore;

namespace ERP.Staccato.Backend.Services;

/// <summary>
/// Authentication service - matches TypeScript auth.service.ts logic
/// </summary>
public class AuthService
{
    private readonly ApplicationDbContext _context;
    private readonly IConfiguration _configuration;

    public AuthService(ApplicationDbContext context, IConfiguration configuration)
    {
        _context = context;
        _configuration = configuration;
    }

    /// <summary>
    /// Login user with username and password
    /// </summary>
    public async Task<LoginResponse> LoginAsync(LoginRequest request)
    {
        try
        {
            // Validate input
            if (string.IsNullOrWhiteSpace(request.User) || string.IsNullOrWhiteSpace(request.Password))
            {
                return new LoginResponse
                {
                    Success = false,
                    Message = "Usuário e senha são obrigatórios"
                };
            }

            // Find user in database
            var usuario = await _context.Set<Usuario>()
                .FirstOrDefaultAsync(u => u.User == request.User && !u.Desativado);

            if (usuario == null)
            {
                return new LoginResponse
                {
                    Success = false,
                    Message = "Usuário ou senha inválidos"
                };
            }

            // Verify password (in production, use bcrypt or similar)
            // For POC, simple comparison
            if (!VerifyPassword(request.Password, usuario.Password))
            {
                return new LoginResponse
                {
                    Success = false,
                    Message = "Usuário ou senha inválidos"
                };
            }

            // Generate JWT token
            var token = GenerateJwtToken(usuario);

            // Return success response
            return new LoginResponse
            {
                Success = true,
                Token = token,
                User = new SessionUser
                {
                    IdUsuario = usuario.IdUsuario,
                    User = usuario.User,
                    Nome = usuario.Nome,
                    Tipo = usuario.Tipo,
                    IdLoja = usuario.IdLoja,
                    Email = usuario.Email
                }
            };
        }
        catch (Exception ex)
        {
            return new LoginResponse
            {
                Success = false,
                Message = $"Erro no login: {ex.Message}"
            };
        }
    }

    /// <summary>
    /// Get current user from token
    /// </summary>
    public async Task<SessionUser?> GetCurrentUserAsync(int userId)
    {
        var usuario = await _context.Set<Usuario>()
            .FirstOrDefaultAsync(u => u.IdUsuario == userId && !u.Desativado);

        if (usuario == null)
            return null;

        return new SessionUser
        {
            IdUsuario = usuario.IdUsuario,
            User = usuario.User,
            Nome = usuario.Nome,
            Tipo = usuario.Tipo,
            IdLoja = usuario.IdLoja,
            Email = usuario.Email
        };
    }

    /// <summary>
    /// Generate JWT token
    /// </summary>
    private string GenerateJwtToken(Usuario usuario)
    {
        var key = _configuration["Jwt:Key"]
            ?? throw new InvalidOperationException("JWT key not configured");

        var securityKey = new SymmetricSecurityKey(Encoding.ASCII.GetBytes(key));
        var credentials = new SigningCredentials(securityKey, SecurityAlgorithms.HmacSha256);

        var claims = new[]
        {
            new Claim(ClaimTypes.NameIdentifier, usuario.IdUsuario.ToString()),
            new Claim(ClaimTypes.Name, usuario.User),
            new Claim("Nome", usuario.Nome),
            new Claim("Tipo", usuario.Tipo),
            new Claim("IdLoja", usuario.IdLoja.ToString())
        };

        var expiration = DateTime.UtcNow.AddMinutes(
            int.Parse(_configuration["Jwt:ExpirationMinutes"] ?? "1440")
        );

        var token = new JwtSecurityToken(
            issuer: _configuration["Jwt:Issuer"],
            audience: _configuration["Jwt:Audience"],
            claims: claims,
            expires: expiration,
            signingCredentials: credentials
        );

        return new JwtSecurityTokenHandler().WriteToken(token);
    }

    /// <summary>
    /// Verify password (placeholder - use bcrypt in production)
    /// </summary>
    private bool VerifyPassword(string password, string hash)
    {
        // For POC: simple comparison
        // In production: use BCrypt.Net-Next or similar
        return password == hash;
    }
}
