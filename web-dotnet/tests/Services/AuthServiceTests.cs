using Xunit;
using FluentAssertions;
using ERP.Staccato.Backend.Services;
using ERP.Staccato.Backend.Tests.Fixtures;
using ERP.Staccato.Backend.Data;
using ERP.Staccato.Shared.Models;
using Microsoft.Extensions.Configuration;
using Moq;

namespace ERP.Staccato.Backend.Tests.Services;

public class AuthServiceTests
{
    private readonly IConfiguration _mockConfiguration;

    public AuthServiceTests()
    {
        var configDict = new Dictionary<string, string>
        {
            { "Jwt:SecretKey", "YXZlcnlsb25nand0c2VjcmV0a2V5dGhhdGlzbG9uZ2Vub3VnaHRvYmVzYWZlYW5kY2FubmJlZWFzaWx5aGFja2VkYW5kaXNmb3J0ZXN0aW5ndXNlb25seQ==" },
            { "Jwt:Issuer", "staccato" },
            { "Jwt:Audience", "staccato-users" },
            { "Jwt:ExpirationMinutes", "60" }
        };
        _mockConfiguration = new ConfigurationBuilder()
            .AddInMemoryCollection(configDict)
            .Build();
    }

    #region Login Tests

    [Fact]
    public async Task LoginAsync_WithValidCredentials_ShouldReturnSuccessWithToken()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var authService = new AuthService(context, _mockConfiguration);
        var request = new LoginRequest
        {
            User = "admin",
            Password = "admin",
            Staging = false
        };

        // Act
        var response = await authService.LoginAsync(request);

        // Assert
        response.Success.Should().BeTrue();
        response.Token.Should().NotBeNullOrEmpty();
        response.User.Should().NotBeNull();
        response.User!.IdUsuario.Should().Be(1);
        response.User.Nome.Should().Be("Administrador");
        response.User.Tipo.Should().Be(SessionUser.Roles.ADMINISTRADOR);
        response.Error.Should().BeNull();
    }

    [Fact]
    public async Task LoginAsync_WithInvalidPassword_ShouldReturnFailure()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var authService = new AuthService(context, _mockConfiguration);
        var request = new LoginRequest
        {
            User = "admin",
            Password = "wrongpassword",
            Staging = false
        };

        // Act
        var response = await authService.LoginAsync(request);

        // Assert
        response.Success.Should().BeFalse();
        response.Error.Should().Contain("Login inválido");
        response.Token.Should().BeNull();
        response.User.Should().BeNull();
    }

    [Fact]
    public async Task LoginAsync_WithNonexistentUser_ShouldReturnFailure()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var authService = new AuthService(context, _mockConfiguration);
        var request = new LoginRequest
        {
            User = "nonexistent",
            Password = "password",
            Staging = false
        };

        // Act
        var response = await authService.LoginAsync(request);

        // Assert
        response.Success.Should().BeFalse();
        response.Error.Should().Contain("Login inválido");
    }

    [Fact]
    public async Task LoginAsync_WithOperacionalUser_ShouldReturnBlocked()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var authService = new AuthService(context, _mockConfiguration);
        var request = new LoginRequest
        {
            User = "operacional",
            Password = "operacional",
            Staging = false
        };

        // Act
        var response = await authService.LoginAsync(request);

        // Assert
        response.Success.Should().BeFalse();
        response.Error.Should().Contain("Operacional bloqueado");
    }

    [Fact]
    public async Task LoginAsync_WhenMaintenanceMode_ShouldReturnMaintenance()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var maintenance = context.Set<Maintenance>().First(m => m.Id == 1);
        maintenance.EmManutencao = true;
        context.SaveChanges();

        var authService = new AuthService(context, _mockConfiguration);
        var request = new LoginRequest
        {
            User = "admin",
            Password = "admin",
            Staging = false
        };

        // Act
        var response = await authService.LoginAsync(request);

        // Assert
        response.Success.Should().BeFalse();
        response.Error.Should().Contain("manutenção");
    }

    [Fact]
    public async Task LoginAsync_WithValidCredentials_ShouldIncludeLoja()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var authService = new AuthService(context, _mockConfiguration);
        var request = new LoginRequest
        {
            User = "admin",
            Password = "admin",
            Staging = false
        };

        // Act
        var response = await authService.LoginAsync(request);

        // Assert
        response.Success.Should().BeTrue();
        response.User.Should().NotBeNull();
        response.User!.Loja.Should().NotBeNull();
        response.User.Loja!.IdLoja.Should().Be(1);
        response.User.Loja.NomeFantasia.Should().Be("Staccato Matriz");
    }

    [Fact]
    public async Task LoginAsync_ShouldGenerateValidJwtToken()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var authService = new AuthService(context, _mockConfiguration);
        var request = new LoginRequest
        {
            User = "admin",
            Password = "admin",
            Staging = false
        };

        // Act
        var response = await authService.LoginAsync(request);

        // Assert
        response.Token.Should().NotBeNullOrEmpty();
        response.Token.Should().Contain(".");
        var tokenParts = response.Token!.Split('.');
        tokenParts.Should().HaveCount(3);
    }

    #endregion

    #region Authorization Tests

    [Fact]
    public async Task AuthorizeAsync_WithValidOneTimePassword_ShouldReturnSuccess()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var authService = new AuthService(context, _mockConfiguration);
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "1234"
        };

        // Act
        var response = await authService.AuthorizeAsync(request);

        // Assert
        response.Success.Should().BeTrue();
        response.ValorMinimoFrete.Should().Be(50m);
        response.Message.Should().BeNull();
    }

    [Fact]
    public async Task AuthorizeAsync_WithInvalidPassword_ShouldReturnFailure()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var authService = new AuthService(context, _mockConfiguration);
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "0000"
        };

        // Act
        var response = await authService.AuthorizeAsync(request);

        // Assert
        response.Success.Should().BeFalse();
        response.Message.Should().NotBeNullOrEmpty();
    }

    [Fact]
    public async Task AuthorizeAsync_ShouldClearOneTimePasswordAfterUse()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var authService = new AuthService(context, _mockConfiguration);
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "1234"
        };

        // Act
        var response = await authService.AuthorizeAsync(request);

        // Assert
        response.Success.Should().BeTrue();
        var usuario = context.Set<Usuario>().First(u => u.IdUsuario == 2);
        usuario.SenhaUsoUnico.Should().BeNull();
    }

    [Fact]
    public async Task AuthorizeAsync_WithVendorUser_ShouldReturnFailure()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var authService = new AuthService(context, _mockConfiguration);
        var request = new AuthorizationRequest
        {
            User = "vendedor",
            SenhaUsoUnico = "1234"
        };

        // Act
        var response = await authService.AuthorizeAsync(request);

        // Assert
        response.Success.Should().BeFalse();
    }

    #endregion

    #region Role Tests

    [Theory]
    [InlineData(SessionUser.Roles.ADMINISTRADOR)]
    [InlineData(SessionUser.Roles.DIRETOR)]
    public void IsAdmin_WithAdminRoles_ShouldReturnTrue(string role)
    {
        RoleHelpers.IsAdmin(role).Should().BeTrue();
    }

    [Fact]
    public void IsAdmin_WithNonAdminRole_ShouldReturnFalse()
    {
        RoleHelpers.IsAdmin(SessionUser.Roles.VENDEDOR).Should().BeFalse();
    }

    [Theory]
    [InlineData(SessionUser.Roles.GERENTE_LOJA)]
    [InlineData(SessionUser.Roles.GERENTE_DEPARTAMENTO)]
    [InlineData(SessionUser.Roles.GERENTE_FINANCEIRO)]
    public void IsGerente_WithGerenteRoles_ShouldReturnTrue(string role)
    {
        RoleHelpers.IsGerente(role).Should().BeTrue();
    }

    [Fact]
    public void IsGerente_WithNonGerenteRole_ShouldReturnFalse()
    {
        RoleHelpers.IsGerente(SessionUser.Roles.VENDEDOR).Should().BeFalse();
    }

    [Theory]
    [InlineData(SessionUser.Roles.VENDEDOR)]
    [InlineData(SessionUser.Roles.VENDEDOR_ESPECIAL)]
    public void IsVendedorOrEspecial_WithVendorRoles_ShouldReturnTrue(string role)
    {
        RoleHelpers.IsVendedorOrEspecial(role).Should().BeTrue();
    }

    [Fact]
    public void CanAuthorize_WithAdminRoles_ShouldReturnTrue()
    {
        RoleHelpers.CanAuthorize(SessionUser.Roles.ADMINISTRADOR).Should().BeTrue();
        RoleHelpers.CanAuthorize(SessionUser.Roles.GERENTE_LOJA).Should().BeTrue();
    }

    [Fact]
    public void CanAuthorize_WithVendorRole_ShouldReturnFalse()
    {
        RoleHelpers.CanAuthorize(SessionUser.Roles.VENDEDOR).Should().BeFalse();
    }

    #endregion
}
