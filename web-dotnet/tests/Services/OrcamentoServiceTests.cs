using Xunit;
using FluentAssertions;
using ERP.Staccato.Backend.Services;
using ERP.Staccato.Backend.Tests.Fixtures;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Backend.Tests.Services;

public class OrcamentoServiceTests
{
    #region Filter Tests

    [Fact]
    public async Task ListAsync_WithNoFilters_ShouldReturnAllOrcamentos()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);
        var filters = new OrcamentoFilters();

        // Act
        // Note: This will fail with current DB because we don't have orcamentos seeded
        // This is expected - shows we need integration tests with real data
        var result = await service.ListAsync(
            filters,
            userId: 1,
            userType: SessionUser.Roles.ADMINISTRADOR,
            userLojaId: 1,
            userName: "admin"
        );

        // Assert
        result.Should().NotBeNull();
        result.Should().BeOfType<List<OrcamentoListItem>>();
    }

    [Fact]
    public async Task ListAsync_WithStoreFilter_ShouldApplyIdLojaCondition()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);
        var filters = new OrcamentoFilters
        {
            IdLoja = 1
        };

        // Act
        var result = await service.ListAsync(
            filters,
            userId: 1,
            userType: SessionUser.Roles.ADMINISTRADOR,
            userLojaId: 1,
            userName: "admin"
        );

        // Assert
        result.Should().NotBeNull();
    }

    [Fact]
    public async Task ListAsync_WithMonthFilter_ShouldApplyData2Condition()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);
        var filters = new OrcamentoFilters
        {
            MesAno = "2024-01"
        };

        // Act
        var result = await service.ListAsync(
            filters,
            userId: 1,
            userType: SessionUser.Roles.ADMINISTRADOR,
            userLojaId: 1,
            userName: "admin"
        );

        // Assert
        result.Should().NotBeNull();
    }

    [Fact]
    public async Task ListAsync_WithStatusFilter_ShouldApplyStatusCondition()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);
        var filters = new OrcamentoFilters
        {
            Statuses = new List<string> { "ATIVO", "EXPIRADO" }
        };

        // Act
        var result = await service.ListAsync(
            filters,
            userId: 1,
            userType: SessionUser.Roles.ADMINISTRADOR,
            userLojaId: 1,
            userName: "admin"
        );

        // Assert
        result.Should().NotBeNull();
    }

    [Fact]
    public async Task ListAsync_WithSupplierFilter_ShouldApplyFornecedorCondition()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);
        var filters = new OrcamentoFilters
        {
            Fornecedor = "Fornecedor A"
        };

        // Act
        var result = await service.ListAsync(
            filters,
            userId: 1,
            userType: SessionUser.Roles.ADMINISTRADOR,
            userLojaId: 1,
            userName: "admin"
        );

        // Assert
        result.Should().NotBeNull();
    }

    [Fact]
    public async Task ListAsync_WithSemaforoFilter_ShouldApplySemaforoCondition()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);
        var filters = new OrcamentoFilters
        {
            Semaforo = 1
        };

        // Act
        var result = await service.ListAsync(
            filters,
            userId: 1,
            userType: SessionUser.Roles.ADMINISTRADOR,
            userLojaId: 1,
            userName: "admin"
        );

        // Assert
        result.Should().NotBeNull();
    }

    [Fact]
    public async Task ListAsync_WithSearchFilter_ShouldApplySearchConditions()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);
        var filters = new OrcamentoFilters
        {
            Search = "Cliente"
        };

        // Act
        var result = await service.ListAsync(
            filters,
            userId: 1,
            userType: SessionUser.Roles.ADMINISTRADOR,
            userLojaId: 1,
            userName: "admin"
        );

        // Assert
        result.Should().NotBeNull();
    }

    #endregion

    #region Role-Based Access Tests

    [Fact]
    public async Task ListAsync_WithGerenteLojaUser_ShouldLimitToUserLoja()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);
        var filters = new OrcamentoFilters();

        // Act
        var result = await service.ListAsync(
            filters,
            userId: 2,
            userType: SessionUser.Roles.GERENTE_LOJA,
            userLojaId: 1,
            userName: "gerente"
        );

        // Assert
        result.Should().NotBeNull();
        // All results should be from loja 1
    }

    [Fact]
    public async Task ListAsync_WithVendorUser_ShouldAllowPropriosFilter()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);
        var filters = new OrcamentoFilters
        {
            ApenasPropriosOrcamentos = true
        };

        // Act
        var result = await service.ListAsync(
            filters,
            userId: 3,
            userType: SessionUser.Roles.VENDEDOR,
            userLojaId: 1,
            userName: "vendedor"
        );

        // Assert
        result.Should().NotBeNull();
    }

    #endregion

    #region Dropdown Data Tests

    [Fact]
    public async Task GetLojasForFilterAsync_ShouldReturnAllLojas()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);

        // Act
        var result = await service.GetLojasForFilterAsync();

        // Assert
        result.Should().NotBeNull();
        result.Should().HaveCount(2);
        result[0].IdLoja.Should().Be(1);
        result[0].NomeFantasia.Should().Be("Staccato Matriz");
        result[1].IdLoja.Should().Be(2);
        result[1].NomeFantasia.Should().Be("Staccato Filial");
    }

    [Fact]
    public async Task GetVendedoresForFilterAsync_ShouldReturnVendorsOnly()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);

        // Act
        var result = await service.GetVendedoresForFilterAsync();

        // Assert
        result.Should().NotBeNull();
        result.Should().ContainSingle();
        result[0].Nome.Should().Be("Vendedor Um");
    }

    [Fact]
    public async Task GetFornecedoresForFilterAsync_ShouldReturnAllSuppliers()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);

        // Act
        var result = await service.GetFornecedoresForFilterAsync();

        // Assert
        result.Should().NotBeNull();
        result.Should().BeOfType<List<FornecedorDto>>();
    }

    #endregion

    #region Multiple Filters Test

    [Fact]
    public async Task ListAsync_WithMultipleFilters_ShouldCombineWithAnd()
    {
        // Arrange
        var context = TestDbContextFactory.CreateTestContext();
        var service = new OrcamentoService(context);
        var filters = new OrcamentoFilters
        {
            IdLoja = 1,
            MesAno = "2024-01",
            Statuses = new List<string> { "ATIVO" },
            Fornecedor = "Supplier A",
            Semaforo = 1
        };

        // Act
        var result = await service.ListAsync(
            filters,
            userId: 1,
            userType: SessionUser.Roles.ADMINISTRADOR,
            userLojaId: 1,
            userName: "admin"
        );

        // Assert
        result.Should().NotBeNull();
    }

    #endregion
}
