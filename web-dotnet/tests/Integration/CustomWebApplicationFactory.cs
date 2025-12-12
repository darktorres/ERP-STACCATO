using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.DependencyInjection;
using ERP.Staccato.Backend;
using ERP.Staccato.Backend.Data;
using ERP.Staccato.Shared.Models;

namespace ERP.Staccato.Backend.Tests.Integration;

/// <summary>
/// Custom WebApplicationFactory for testing the API
/// Replaces real database with in-memory test database
/// </summary>
public class CustomWebApplicationFactory : WebApplicationFactory<Program>
{
    protected override void ConfigureWebHost(IWebHostBuilder builder)
    {
        builder.ConfigureServices(services =>
        {
            // Remove the default DbContext
            var descriptor = services.SingleOrDefault(d =>
                d.ServiceType == typeof(DbContextOptions<ApplicationDbContext>));

            if (descriptor != null)
            {
                services.Remove(descriptor);
            }

            // Add real MySQL database for testing
            var connectionString = "Server=localhost;Database=staccato;User=root;Password=1234;";
            services.AddDbContext<ApplicationDbContext>(options =>
            {
                options.UseMySql(
                    connectionString,
                    ServerVersion.AutoDetect(connectionString)
                );
            });

            // Build service provider and seed test data
            var sp = services.BuildServiceProvider();
            using (var scope = sp.CreateScope())
            {
                var dbContext = scope.ServiceProvider.GetRequiredService<ApplicationDbContext>();
                dbContext.Database.EnsureCreated();
                SeedTestData(dbContext);
            }
        });
    }

    private static void SeedTestData(ApplicationDbContext context)
    {
        // Clear existing data
        context.Set<Usuario>().RemoveRange(context.Set<Usuario>());
        context.Set<Loja>().RemoveRange(context.Set<Loja>());
        context.Set<Maintenance>().RemoveRange(context.Set<Maintenance>());
        context.SaveChanges();

        // Seed test data - all users use password "1234"
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
            }
        };

        var lojas = new[]
        {
            new Loja
            {
                IdLoja = 1,
                Descricao = "Loja Principal",
                NomeFantasia = "Staccato Matriz"
            }
        };

        var maintenance = new Maintenance
        {
            Id = 1,
            Created = DateTime.UtcNow,
            LastUpdated = DateTime.UtcNow,
            EmManutencao = false
        };

        context.Set<Usuario>().AddRange(usuarios);
        context.Set<Loja>().AddRange(lojas);
        context.Set<Maintenance>().Add(maintenance);
        context.SaveChanges();
    }
}
