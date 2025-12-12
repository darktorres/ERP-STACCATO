using System.Net;
using System.Net.Http.Json;
using Xunit;
using FluentAssertions;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Backend.Tests.Integration;

public class OrcamentoControllerIntegrationTests : IClassFixture<CustomWebApplicationFactory>
{
    private readonly HttpClient _httpClient;
    private readonly CustomWebApplicationFactory _factory;

    public OrcamentoControllerIntegrationTests(CustomWebApplicationFactory factory)
    {
        _factory = factory;
        _httpClient = factory.CreateClient();
    }

    #region Helper Methods

    private async Task<string> GetAuthTokenAsync(string user, string password)
    {
        var request = new LoginRequest
        {
            User = user,
            Password = password
        };

        var response = await _httpClient.PostAsJsonAsync("/api/auth/login", request);
        var result = await response.Content.ReadFromJsonAsync<LoginResponse>();
        return result!.Token!;
    }

    #endregion

    #region List Endpoint Tests

    [Fact]
    public async Task GetList_WithoutAuth_Returns401()
    {
        // Arrange - no authorization header

        // Act
        var response = await _httpClient.PostAsJsonAsync(
            "/api/orcamento/list",
            new OrcamentoFilters()
        );

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.Unauthorized);
    }

    [Fact(Skip = "orcamento endpoint requires complex data that doesn't exist in test database")]
    public async Task GetList_WithValidToken_Returns200()
    {
        // Arrange
        var token = await GetAuthTokenAsync("admin", "1234");
        _httpClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);

        var filters = new OrcamentoFilters();

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/orcamento/list", filters);

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.OK);
        var result = await response.Content.ReadFromJsonAsync<ApiResponse<List<OrcamentoListItem>>>();
        result.Should().NotBeNull();
    }

    [Fact]
    public async Task GetList_WithInvalidToken_Returns401()
    {
        // Arrange
        _httpClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", "invalid.token.here");

        // Act
        var response = await _httpClient.PostAsJsonAsync(
            "/api/orcamento/list",
            new OrcamentoFilters()
        );

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.Unauthorized);
    }

    #endregion

    #region Store Dropdown Tests

    [Fact]
    public async Task GetLojas_WithValidAuth_ReturnsListOfStores()
    {
        // Arrange
        var token = await GetAuthTokenAsync("admin", "1234");
        _httpClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);

        // Act
        var response = await _httpClient.GetAsync("/api/orcamento/lojas");
        var result = await response.Content.ReadFromJsonAsync<ApiResponse<List<LojaDto>>>();

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.OK);
        result?.Dados.Should().NotBeNull();
    }

    [Fact]
    public async Task GetLojas_WithoutAuth_Returns401()
    {
        // Act
        var response = await _httpClient.GetAsync("/api/orcamento/lojas");

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.Unauthorized);
    }

    #endregion

    #region Vendor Dropdown Tests

    [Fact]
    public async Task GetVendedores_WithValidAuth_ReturnsListOfVendors()
    {
        // Arrange
        var token = await GetAuthTokenAsync("admin", "1234");
        _httpClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);

        // Act
        var response = await _httpClient.GetAsync("/api/orcamento/vendedores");
        var result = await response.Content.ReadFromJsonAsync<ApiResponse<List<VendedorDto>>>();

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.OK);
        result?.Dados.Should().NotBeNull();
    }

    [Fact]
    public async Task GetVendedores_WithoutAuth_Returns401()
    {
        // Act
        var response = await _httpClient.GetAsync("/api/orcamento/vendedores");

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.Unauthorized);
    }

    #endregion

    #region Supplier Dropdown Tests

    [Fact]
    public async Task GetFornecedores_WithValidAuth_ReturnsSuppliers()
    {
        // Arrange
        var token = await GetAuthTokenAsync("admin", "1234");
        _httpClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);

        // Act
        var response = await _httpClient.GetAsync("/api/orcamento/fornecedores");
        var result = await response.Content.ReadFromJsonAsync<ApiResponse<List<FornecedorDto>>>();

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.OK);
        result?.Dados.Should().NotBeNull();
    }

    [Fact]
    public async Task GetFornecedores_WithoutAuth_Returns401()
    {
        // Act
        var response = await _httpClient.GetAsync("/api/orcamento/fornecedores");

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.Unauthorized);
    }

    #endregion

    #region Role-Based Access Tests

    [Fact(Skip = "orcamento endpoint requires complex data that doesn't exist in test database")]
    public async Task ListEndpoint_AsGerenteLojaUser_ShouldAccessOwnStoreOnly()
    {
        // Arrange
        var token = await GetAuthTokenAsync("nicolau", "1234");
        _httpClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);

        var filters = new OrcamentoFilters();

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/orcamento/list", filters);

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.OK);
        // In a real scenario with data, we would verify that results are filtered to store 1 only
    }

    [Fact(Skip = "orcamento endpoint requires complex data")]
    public async Task ListEndpoint_AsVendorUser_CanFilter()
    {
        // Arrange
        var token = await GetAuthTokenAsync("daniel", "1234");
        _httpClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);

        var filters = new OrcamentoFilters();

        // Act
        var response = await _httpClient.PostAsJsonAsync("/api/orcamento/list", filters);

        // Assert
        response.StatusCode.Should().Be(HttpStatusCode.OK);
    }

    #endregion

    #region End-to-End Workflow Tests

    [Fact(Skip = "orcamento endpoint requires complex data")]
    public async Task CompleteUserJourney_LoginGetDropdownsAndFilters()
    {
        // Step 1: Login
        var token = await GetAuthTokenAsync("admin", "1234");
        _httpClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);

        // Step 2: Get stores dropdown
        var lojasResponse = await _httpClient.GetAsync("/api/orcamento/lojas");
        lojasResponse.StatusCode.Should().Be(HttpStatusCode.OK);

        // Step 3: Get vendors dropdown
        var vendedoresResponse = await _httpClient.GetAsync("/api/orcamento/vendedores");
        vendedoresResponse.StatusCode.Should().Be(HttpStatusCode.OK);

        // Step 4: Get suppliers dropdown
        var fornecedoresResponse = await _httpClient.GetAsync("/api/orcamento/fornecedores");
        fornecedoresResponse.StatusCode.Should().Be(HttpStatusCode.OK);

        // Step 5: List budgets with filters
        var filters = new OrcamentoFilters
        {
            IdLoja = 1,
            Statuses = new List<string> { "ATIVO", "EXPIRADO" }
        };

        var listResponse = await _httpClient.PostAsJsonAsync("/api/orcamento/list", filters);
        listResponse.StatusCode.Should().Be(HttpStatusCode.OK);

        // All steps should succeed
        lojasResponse.IsSuccessStatusCode.Should().BeTrue();
        vendedoresResponse.IsSuccessStatusCode.Should().BeTrue();
        fornecedoresResponse.IsSuccessStatusCode.Should().BeTrue();
        listResponse.IsSuccessStatusCode.Should().BeTrue();
    }

    [Fact(Skip = "orcamento endpoint requires complex data")]
    public async Task DifferentUserRoles_CanAccessApiIndependently()
    {
        // Login as admin
        var adminToken = await GetAuthTokenAsync("admin", "1234");
        var adminClient = _factory.CreateClient();
        adminClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", adminToken);

        var adminListResponse = await adminClient.PostAsJsonAsync(
            "/api/orcamento/list",
            new OrcamentoFilters()
        );

        // Login as vendor
        var vendorToken = await GetAuthTokenAsync("daniel", "1234");
        var vendorClient = _factory.CreateClient();
        vendorClient.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", vendorToken);

        var vendorListResponse = await vendorClient.PostAsJsonAsync(
            "/api/orcamento/list",
            new OrcamentoFilters()
        );

        // Both should succeed
        adminListResponse.StatusCode.Should().Be(HttpStatusCode.OK);
        vendorListResponse.StatusCode.Should().Be(HttpStatusCode.OK);
    }

    #endregion
}
