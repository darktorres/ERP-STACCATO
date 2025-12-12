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
    /// Login user with user and password
    /// Matches TypeScript auth.service.ts login() method (lines 18-93)
    /// </summary>
    public async Task<LoginResponse> LoginAsync(LoginRequest request)
    {
        try
        {
            // STEP 1: Check maintenance mode
            var maintenance = await _context.Set<Maintenance>()
                .FirstOrDefaultAsync(m => m.Id == 1);

            if (maintenance?.EmManutencao == true)
            {
                return new LoginResponse
                {
                    Success = false,
                    Error = "Sistema em manutenção!"
                };
            }

            // STEP 2: Query user with SHA_PASSWORD comparison
            // Use raw SQL to match MySQL SHA_PASSWORD function
            var sql = @"
                SELECT idUsuario, idLoja, nome, tipo, desativado
                FROM usuario
                WHERE user = {0}
                  AND password = SHA_PASSWORD({1})
                  AND desativado = FALSE
            ";

            var usuarios = await _context.Set<Usuario>()
                .FromSqlRaw(sql, request.User.ToLower(), request.Password)
                .Select(u => new
                {
                    u.IdUsuario,
                    u.IdLoja,
                    u.Nome,
                    u.Tipo,
                    u.Desativado
                })
                .ToListAsync();

            if (usuarios.Count == 0)
            {
                return new LoginResponse
                {
                    Success = false,
                    Error = "Login inválido!"
                };
            }

            var usuario = usuarios[0];

            // STEP 3: Block OPERACIONAL users
            if (usuario.Tipo == SessionUser.Roles.OPERACIONAL)
            {
                return new LoginResponse
                {
                    Success = false,
                    Error = "Operacional bloqueado!"
                };
            }

            // STEP 4: Get loja info
            var loja = await _context.Set<Loja>()
                .Where(l => l.IdLoja == usuario.IdLoja)
                .Select(l => new LojaInfo
                {
                    IdLoja = l.IdLoja,
                    Descricao = l.Descricao ?? string.Empty,
                    NomeFantasia = l.NomeFantasia ?? string.Empty
                })
                .FirstOrDefaultAsync();

            // STEP 5: Create session user
            var sessionUser = new SessionUser
            {
                IdUsuario = usuario.IdUsuario,
                IdLoja = usuario.IdLoja,
                User = request.User.ToLower(),
                Tipo = usuario.Tipo,
                Nome = usuario.Nome
            };

            // STEP 6: Generate JWT token
            var token = GenerateJwtToken(sessionUser);

            // STEP 7: Return success with user + loja
            return new LoginResponse
            {
                Success = true,
                Token = token,
                User = new SessionUserWithLoja
                {
                    IdUsuario = sessionUser.IdUsuario,
                    IdLoja = sessionUser.IdLoja,
                    User = sessionUser.User,
                    Tipo = sessionUser.Tipo,
                    Nome = sessionUser.Nome,
                    Loja = loja
                }
            };
        }
        catch (Exception ex)
        {
            return new LoginResponse
            {
                Success = false,
                Error = $"Erro no login: {ex.Message}"
            };
        }
    }

    /// <summary>
    /// Authorization with one-time password
    /// Matches TypeScript auth.service.ts authorize() method (lines 99-135)
    /// </summary>
    public async Task<AuthorizationResponse> AuthorizeAsync(AuthorizationRequest request)
    {
        try
        {
            // Query for authorization - managers/admins only
            var sql = @"
                SELECT idUsuario, valorMinimoFrete
                FROM usuario
                WHERE user = {0}
                  AND senhaUsoUnico = {1}
                  AND tipo IN ('ADMINISTRADOR', 'ADMINISTRATIVO', 'DIRETOR',
                              'GERENTE DEPARTAMENTO', 'GERENTE LOJA')
            ";

            var usuarios = await _context.Database
                .SqlQueryRaw<dynamic>(sql, request.User, request.SenhaUsoUnico)
                .ToListAsync();

            if (usuarios.Count == 0)
            {
                return new AuthorizationResponse
                {
                    Success = false,
                    Message = "Senha não confere!"
                };
            }

            var usuario = usuarios[0];
            var idUsuario = (int)usuario.idUsuario;
            var valorMinimoFrete = usuario.valorMinimoFrete as decimal?;

            // Clear one-time password after use
            var userEntity = await _context.Set<Usuario>()
                .FirstOrDefaultAsync(u => u.IdUsuario == idUsuario);

            if (userEntity != null)
            {
                userEntity.SenhaUsoUnico = null;
                userEntity.ValorMinimoFrete = null;
                await _context.SaveChangesAsync();
            }

            return new AuthorizationResponse
            {
                Success = true,
                ValorMinimoFrete = valorMinimoFrete
            };
        }
        catch (Exception ex)
        {
            return new AuthorizationResponse
            {
                Success = false,
                Message = $"Erro na autorização: {ex.Message}"
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
    private string GenerateJwtToken(SessionUser usuario)
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
}
