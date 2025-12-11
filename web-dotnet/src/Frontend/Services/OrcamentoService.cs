using System.Net.Http.Json;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Frontend.Services;

/// <summary>
/// Frontend orcamento (budget) service
/// </summary>
public class OrcamentoService
{
    private readonly HttpClient _httpClient;
    private readonly ILogger<OrcamentoService> _logger;

    public OrcamentoService(HttpClient httpClient, ILogger<OrcamentoService> logger)
    {
        _httpClient = httpClient;
        _logger = logger;
    }

    /// <summary>
    /// List orcamentos with filters
    /// </summary>
    public async Task<List<OrcamentoListItem>> ListAsync(OrcamentoFilters filters)
    {
        try
        {
            var response = await _httpClient.PostAsJsonAsync("/api/orcamento/list", filters);

            if (!response.IsSuccessStatusCode)
            {
                _logger.LogWarning($"List failed with status {response.StatusCode}");
                return new();
            }

            var result = await response.Content.ReadAsAsync<ApiResponse<List<OrcamentoListItem>>>();
            return result?.Data ?? new();
        }
        catch (Exception ex)
        {
            _logger.LogError($"List error: {ex.Message}");
            return new();
        }
    }

    /// <summary>
    /// Get lojas for dropdown
    /// </summary>
    public async Task<List<LojaDto>> GetLojasAsync()
    {
        try
        {
            var response = await _httpClient.GetAsync("/api/orcamento/lojas");

            if (!response.IsSuccessStatusCode)
                return new();

            var result = await response.Content.ReadAsAsync<ApiResponse<List<LojaDto>>>();
            return result?.Data ?? new();
        }
        catch (Exception ex)
        {
            _logger.LogError($"GetLojas error: {ex.Message}");
            return new();
        }
    }

    /// <summary>
    /// Get vendedores for dropdown
    /// </summary>
    public async Task<List<VendedorDto>> GetVendedoresAsync(int? idLoja = null)
    {
        try
        {
            var url = "/api/orcamento/vendedores";
            if (idLoja.HasValue)
                url += $"?idLoja={idLoja}";

            var response = await _httpClient.GetAsync(url);

            if (!response.IsSuccessStatusCode)
                return new();

            var result = await response.Content.ReadAsAsync<ApiResponse<List<VendedorDto>>>();
            return result?.Data ?? new();
        }
        catch (Exception ex)
        {
            _logger.LogError($"GetVendedores error: {ex.Message}");
            return new();
        }
    }

    /// <summary>
    /// Get fornecedores for dropdown
    /// </summary>
    public async Task<List<FornecedorDto>> GetFornecedoresAsync()
    {
        try
        {
            var response = await _httpClient.GetAsync("/api/orcamento/fornecedores");

            if (!response.IsSuccessStatusCode)
                return new();

            var result = await response.Content.ReadAsAsync<ApiResponse<List<FornecedorDto>>>();
            return result?.Data ?? new();
        }
        catch (Exception ex)
        {
            _logger.LogError($"GetFornecedores error: {ex.Message}");
            return new();
        }
    }
}
