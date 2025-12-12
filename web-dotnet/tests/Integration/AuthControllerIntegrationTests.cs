using System.Net;
using System.Net.Http.Json;
using Xunit;
using FluentAssertions;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Backend.Tests.Integration;

public class AuthControllerIntegrationTests : IClassFixture<CustomWebApplicationFactory>
{
    private readonly HttpClient _httpClient;

    public AuthControllerIntegrationTests(CustomWebApplicationFactory factory)
    {
        _httpClient = factory.CreateClient();
    }

    #region Login Endpoint Tests

    [Fact]
    public async Task PostLogin_WithValidCredentials_Returns200AndToken()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "admin",
            Password = "1234",
            Staging = false
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);
        var result = await response.Content.ReadFromJsonAsync<LoginResponse>();

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.OK);
        result.Should().NotBeNull();
        result!.Success.Should().BeTrue();
        result.Token.Should().NotBeNullOrEmpty();
        result.User.Should().NotBeNull();
        result.User!.IdUsuario.Should().Be(1);
        result.User.Nome.Should().Be("Administrador");
        result.Error.Should().BeNull();
    }

    [Fact]
    public async Task PostLogin_WithInvalidPassword_Returns401()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "admin",
            Password = "wrongpassword",
            Staging = false
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);
        var result = await response.Content.ReadFromJsonAsync<LoginResponse>();

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.Unauthorized);
        result.Should().NotBeNull();
        result!.Success.Should().BeFalse();
        result.Token.Should().BeNull();
        result.Error.Should().NotBeNullOrEmpty();
    }

    [Fact]
    public async Task PostLogin_WithNonexistentUser_Returns401()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "nonexistent",
            Password = "password",
            Staging = false
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);
        var result = await response.Content.ReadFromJsonAsync<LoginResponse>();

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.Unauthorized);
        result!.Success.Should().BeFalse();
    }

    [Fact]
    public async Task PostLogin_WithEmptyUser_ReturnsBadRequest()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "",
            Password = "1234"
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.BadRequest);
    }

    [Fact]
    public async Task PostLogin_WithShortPassword_ReturnsBadRequest()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "admin",
            Password = "123"
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.BadRequest);
    }

    [Fact]
    public async Task PostLogin_ReturnsUserWithLoja()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "admin",
            Password = "1234",
            Staging = false
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);
        var result = await response.Content.ReadFromJsonAsync<LoginResponse>();

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.OK);
        result!.User.Should().NotBeNull();
        result.User!.Loja.Should().NotBeNull();
        result.User.Loja!.IdLoja.Should().Be(1);
        result.User.Loja.NomeFantasia.Should().Be("Staccato Matriz");
    }

    [Fact]
    public async Task PostLogin_GeneratesValidJwtToken()
    {
        // Arrange
        var request = new LoginRequest
        {
            User = "admin",
            Password = "1234"
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);
        var result = await response.Content.ReadFromJsonAsync<LoginResponse>();

        // Assert
        result!.Token.Should().NotBeNullOrEmpty();
        var parts = result.Token!.Split('.');
        parts.Should().HaveCount(3);
    }

    #endregion

    #region Authorization Endpoint Tests

    [Fact]
    public async Task PostAuthorize_WithValidPassword_Returns200()
    {
        // Arrange
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "1234"
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/authorize", request);
        var result = await response.Content.ReadFromJsonAsync<AuthorizationResponse>();

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.OK);
        result.Should().NotBeNull();
        result!.Success.Should().BeTrue();
        result.ValorMinimoFrete.Should().Be(50m);
        result.Message.Should().BeNull();
    }

    [Fact]
    public async Task PostAuthorize_WithInvalidPassword_Returns401()
    {
        // Arrange
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "0000"
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/authorize", request);
        var result = await response.Content.ReadFromJsonAsync<AuthorizationResponse>();

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.Unauthorized);
        result!.Success.Should().BeFalse();
        result.Message.Should().NotBeNullOrEmpty();
    }

    [Fact]
    public async Task PostAuthorize_WithWrongPasswordLength_ReturnsBadRequest()
    {
        // Arrange
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "12345"
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/authorize", request);

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.BadRequest);
    }

    [Fact]
    public async Task PostAuthorize_WithNonNumericPassword_ReturnsBadRequest()
    {
        // Arrange
        var request = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "abcd"
        };

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/auth/authorize", request);

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.BadRequest);
    }

    #endregion

    #region End-to-End Workflow Tests

    [Fact]
    public async Task LoginThenAuthorize_CompleteWorkflow_ShouldSucceed()
    {
        // Step 1: Login as gerente
        var loginRequest = new LoginRequest
        {
            User = "gerente",
            Password = "1234",
            Staging = false
        };

        var loginResponse = await _httpClient.PostAsJsonAsync("/api/auth/login", loginRequest);
        var loginResult = await loginResponse.Content.ReadFromJsonAsync<LoginResponse>();

        loginResponse.StatusCode.Should().Be(HttpStatusCode.OK);
        loginResult!.Success.Should().BeTrue();
        var token = loginResult.Token;

        // Step 2: Use token in Authorization header
        _httpClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);

        // Step 3: Authorize with one-time password
        var authRequest = new AuthorizationRequest
        {
            User = "gerente",
            SenhaUsoUnico = "1234"
        };

        var authResponse = await _httpClient.PostAsJsonAsync("/api/auth/authorize", authRequest);
        var authResult = await authResponse.Content.ReadFromJsonAsync<AuthorizationResponse>();

        // Assert
        authResponse.StatusCode.Should().Be(HttpStatusCode.OK);
        authResult!.Success.Should().BeTrue();
        authResult.ValorMinimoFrete.Should().Be(50m);
    }

    [Fact]
    public async Task MultipleLoginAttempts_ShouldWorkIndependently()
    {
        // Login as admin
        var adminLogin = new LoginRequest
        {
            User = "admin",
            Password = "1234"
        };

        var adminResponse = await _httpClient.PostAsJsonAsync("/api/auth/login", adminLogin);
        adminResponse.StatusCode.Should().Be(HttpStatusCode.OK);

        // Login as gerente
        var gerenteLogin = new LoginRequest
        {
            User = "gerente",
            Password = "1234"
        };

        var gerenteResponse = await _httpClient.PostAsJsonAsync("/api/auth/login", gerenteLogin);
        gerenteResponse.StatusCode.Should().Be(HttpStatusCode.OK);

        // Both should succeed
        var adminResult = await adminResponse.Content.ReadFromJsonAsync<LoginResponse>();
        var gerenteResult = await gerenteResponse.Content.ReadFromJsonAsync<LoginResponse>();

        adminResult!.User!.IdUsuario.Should().Be(1);
        gerenteResult!.User!.IdUsuario.Should().Be(2);
    }

    #endregion
}
