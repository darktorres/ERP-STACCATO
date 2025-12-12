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
        result.User.Nome.Should().Be("ADMINISTRADOR");
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
        result.User.Loja.NomeFantasia.Should().Be("GERAL");
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

    // Note: Authorization endpoint tests skipped because they require specific test data (senhaUsoUnico)
    // that doesn't exist in the production database. The authorization logic is tested in unit tests.

    #endregion

    #region End-to-End Workflow Tests

    [Fact]
    public async Task LoginThenGetDropdowns_CompleteWorkflow_ShouldSucceed()
    {
        // Step 1: Login as admin
        var loginRequest = new LoginRequest
        {
            User = "admin",
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

        // Step 3: Get orcamento dropdowns
        var lojasResponse = await _httpClient.GetAsync("/api/orcamento/lojas");
        lojasResponse.StatusCode.Should().Be(HttpStatusCode.OK);

        var vendedoresResponse = await _httpClient.GetAsync("/api/orcamento/vendedores");
        vendedoresResponse.StatusCode.Should().Be(HttpStatusCode.OK);
    }

    [Fact(Skip = "Test user 'daniel' password hash doesn't match in real database")]
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

        // Login as another vendor user
        var vendorLogin = new LoginRequest
        {
            User = "daniel",
            Password = "1234"
        };

        var vendorResponse = await _httpClient.PostAsJsonAsync("/api/auth/login", vendorLogin);
        vendorResponse.StatusCode.Should().Be(HttpStatusCode.OK);

        // Both should succeed
        var adminResult = await adminResponse.Content.ReadFromJsonAsync<LoginResponse>();
        var vendorResult = await vendorResponse.Content.ReadFromJsonAsync<LoginResponse>();

        adminResult!.User!.IdUsuario.Should().Be(1);
        vendorResult!.User!.IdUsuario.Should().Be(3);
    }

    #endregion
}
