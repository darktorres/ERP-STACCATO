using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using System.Security.Claims;
using ERP.Staccato.Backend.Services;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Backend.Controllers;

[ApiController]
[Route("api/[controller]")]
[Authorize]
public class OrcamentoController : ControllerBase
{
    private readonly OrcamentoService _orcamentoService;
    private readonly ILogger<OrcamentoController> _logger;

    public OrcamentoController(OrcamentoService orcamentoService, ILogger<OrcamentoController> logger)
    {
        _orcamentoService = orcamentoService;
        _logger = logger;
    }

    /// <summary>
    /// List orcamentos with filtering
    /// </summary>
    [HttpPost("list")]
    public async Task<ActionResult<ApiResponse<List<OrcamentoListItem>>>> List([FromBody] OrcamentoFilters filters)
    {
        try
        {
            // Get user info from JWT token
            var userId = int.Parse(User.FindFirst(ClaimTypes.NameIdentifier)?.Value ?? "0");
            var userType = User.FindFirst("Tipo")?.Value ?? string.Empty;
            var userLojaId = int.Parse(User.FindFirst("IdLoja")?.Value ?? "0");
            var userName = User.FindFirst("Nome")?.Value ?? string.Empty;

            _logger.LogInformation($"[Orcamento.List] User {userId} ({userType}) requesting budgets");

            var orcamentos = await _orcamentoService.ListAsync(filters, userId, userType, userLojaId, userName);

            _logger.LogInformation($"[Orcamento.List] Returned {orcamentos.Count} budgets");

            return Ok(ApiResponse<List<OrcamentoListItem>>.Ok(orcamentos));
        }
        catch (Exception ex)
        {
            _logger.LogError($"[Orcamento.List] Error: {ex.Message}");
            return StatusCode(500, ApiResponse<List<OrcamentoListItem>>.Error($"Error listing orcamentos: {ex.Message}"));
        }
    }

    /// <summary>
    /// Get lojas for filter dropdown
    /// </summary>
    [HttpGet("lojas")]
    public async Task<ActionResult<ApiResponse<List<LojaDto>>>> GetLojas()
    {
        try
        {
            var lojas = await _orcamentoService.GetLojasForFilterAsync();
            return Ok(ApiResponse<List<LojaDto>>.Ok(lojas));
        }
        catch (Exception ex)
        {
            _logger.LogError($"[Orcamento.GetLojas] Error: {ex.Message}");
            return StatusCode(500, ApiResponse<List<LojaDto>>.Error("Error loading lojas"));
        }
    }

    /// <summary>
    /// Get vendedores for filter dropdown
    /// </summary>
    [HttpGet("vendedores")]
    public async Task<ActionResult<ApiResponse<List<VendedorDto>>>> GetVendedores([FromQuery] int? idLoja = null)
    {
        try
        {
            var vendedores = await _orcamentoService.GetVendedoresForFilterAsync(idLoja);
            return Ok(ApiResponse<List<VendedorDto>>.Ok(vendedores));
        }
        catch (Exception ex)
        {
            _logger.LogError($"[Orcamento.GetVendedores] Error: {ex.Message}");
            return StatusCode(500, ApiResponse<List<VendedorDto>>.Error("Error loading vendedores"));
        }
    }

    /// <summary>
    /// Get fornecedores for filter dropdown
    /// </summary>
    [HttpGet("fornecedores")]
    public async Task<ActionResult<ApiResponse<List<FornecedorDto>>>> GetFornecedores()
    {
        try
        {
            var fornecedores = await _orcamentoService.GetFornecedoresForFilterAsync();
            return Ok(ApiResponse<List<FornecedorDto>>.Ok(fornecedores));
        }
        catch (Exception ex)
        {
            _logger.LogError($"[Orcamento.GetFornecedores] Error: {ex.Message}");
            return StatusCode(500, ApiResponse<List<FornecedorDto>>.Error("Error loading fornecedores"));
        }
    }
}
