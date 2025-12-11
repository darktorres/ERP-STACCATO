using Microsoft.EntityFrameworkCore;

namespace ERP.Staccato.Backend.Data;

/// <summary>
/// Entity Framework Core database context
/// Maps to existing C++ database schema
/// </summary>
public class ApplicationDbContext : DbContext
{
    public ApplicationDbContext(DbContextOptions<ApplicationDbContext> options) : base(options)
    {
    }

    // DbSets will be configured for existing tables
    // For now, we use raw SQL queries to work with existing schema

    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        base.OnModelCreating(modelBuilder);

        // Configure for existing database (Database-First approach)
        // Tables: usuario, loja, orcamento, view_orcamento, etc.

        // Example: Configure Usuario table
        modelBuilder.Entity<Usuario>(entity =>
        {
            entity.HasKey(e => e.IdUsuario);
            entity.ToTable("usuario");

            entity.Property(e => e.IdUsuario).HasColumnName("id_usuario");
            entity.Property(e => e.User).HasColumnName("user");
            entity.Property(e => e.Password).HasColumnName("password");
            entity.Property(e => e.Nome).HasColumnName("nome");
            entity.Property(e => e.Tipo).HasColumnName("tipo");
            entity.Property(e => e.IdLoja).HasColumnName("id_loja");
            entity.Property(e => e.Email).HasColumnName("email");
            entity.Property(e => e.Desativado).HasColumnName("desativado");
        });

        // Configure Loja table
        modelBuilder.Entity<Loja>(entity =>
        {
            entity.HasKey(e => e.IdLoja);
            entity.ToTable("loja");

            entity.Property(e => e.IdLoja).HasColumnName("id_loja");
            entity.Property(e => e.Descricao).HasColumnName("descricao");
            entity.Property(e => e.NomeFantasia).HasColumnName("nome_fantasia");
            entity.Property(e => e.Desativado).HasColumnName("desativado");
        });
    }
}

/// <summary>
/// Usuario entity - mirrors C++ Usuario model
/// </summary>
public class Usuario
{
    public int IdUsuario { get; set; }
    public string User { get; set; } = string.Empty;
    public string Password { get; set; } = string.Empty;
    public string Nome { get; set; } = string.Empty;
    public string Tipo { get; set; } = string.Empty;
    public int IdLoja { get; set; }
    public string? Email { get; set; }
    public bool Desativado { get; set; }
}

/// <summary>
/// Loja entity - mirrors C++ Loja model
/// </summary>
public class Loja
{
    public int IdLoja { get; set; }
    public string? Descricao { get; set; }
    public string? NomeFantasia { get; set; }
    public bool Desativado { get; set; }
}
