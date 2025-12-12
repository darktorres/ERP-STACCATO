using Microsoft.EntityFrameworkCore;
using ERP.Staccato.Backend.Data;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Backend.Tests.Fixtures;

/// <summary>
/// Factory for creating test database contexts with seed data
/// </summary>
public static class TestDbContextFactory
{
    public static ApplicationDbContext CreateTestContext()
    {
        var options = new DbContextOptionsBuilder<ApplicationDbContext>()
            .UseInMemoryDatabase(databaseName: Guid.NewGuid().ToString())
            .Options;

        var context = new ApplicationDbContext(options);
        context.Database.EnsureCreated();
        SeedTestData(context);
        return context;
    }

    private static void SeedTestData(ApplicationDbContext context)
    {
        // Create test users - all use password "1234"
        var usuarios = new[]
        {
            new Usuario
            {
                IdUsuario = 1,
                IdLoja = 1,
                User = "admin",
                Password = "1234",
                Nome = "Administrador",
                Tipo = SessionUser.Roles.ADMINISTRADOR,
                Email = "admin@staccato.com.br",
                Desativado = false,
                SenhaUsoUnico = null,
                ValorMinimoFrete = null
            },
            new Usuario
            {
                IdUsuario = 2,
                IdLoja = 1,
                User = "gerente",
                Password = "1234",
                Nome = "Gerente Loja",
                Tipo = SessionUser.Roles.GERENTE_LOJA,
                Email = "gerente@staccato.com.br",
                Desativado = false,
                SenhaUsoUnico = "1234",
                ValorMinimoFrete = 50m
            },
            new Usuario
            {
                IdUsuario = 3,
                IdLoja = 1,
                User = "vendedor",
                Password = "1234",
                Nome = "Vendedor Um",
                Tipo = SessionUser.Roles.VENDEDOR,
                Email = "vendedor@staccato.com.br",
                Desativado = false,
                SenhaUsoUnico = null,
                ValorMinimoFrete = null
            },
            new Usuario
            {
                IdUsuario = 4,
                IdLoja = 2,
                User = "operacional",
                Password = "1234",
                Nome = "Operacional",
                Tipo = SessionUser.Roles.OPERACIONAL,
                Email = "operacional@staccato.com.br",
                Desativado = false,
                SenhaUsoUnico = null,
                ValorMinimoFrete = null
            }
        };

        context.Set<Usuario>().AddRange(usuarios);

        // Create test stores (lojas)
        var lojas = new[]
        {
            new Loja
            {
                IdLoja = 1,
                Descricao = "Loja Principal",
                NomeFantasia = "Staccato Matriz"
            },
            new Loja
            {
                IdLoja = 2,
                Descricao = "Loja Filial",
                NomeFantasia = "Staccato Filial"
            }
        };

        context.Set<Loja>().AddRange(lojas);

        // Create maintenance record
        var maintenance = new Maintenance
        {
            Id = 1,
            Created = DateTime.UtcNow,
            LastUpdated = DateTime.UtcNow,
            EmManutencao = false
        };

        context.Set<Maintenance>().Add(maintenance);

        context.SaveChanges();
    }
}
