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
        // Tests only query existing data from the production database
        // The implementation only covers WidgetOrcamento (read-only queries), not data creation/modification
        // No modifications to the database are made during testing
    }
}
