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

    public DbSet<OrcamentoView> OrcamentoViews => Set<OrcamentoView>();

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
            entity.Property(e => e.SenhaUsoUnico).HasColumnName("senhaUsoUnico");
            entity.Property(e => e.ValorMinimoFrete).HasColumnName("valorMinimoFrete");
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

        // Configure Maintenance table
        modelBuilder.Entity<Maintenance>(entity =>
        {
            entity.HasKey(e => e.Id);
            entity.ToTable("maintenance");

            entity.Property(e => e.Id).HasColumnName("id");
            entity.Property(e => e.LastInvalidated).HasColumnName("lastInvalidated");
            entity.Property(e => e.UltimaConsultaNSU).HasColumnName("ultimaConsultaNSU");
            entity.Property(e => e.EmManutencao).HasColumnName("emManutencao");
            entity.Property(e => e.Created).HasColumnName("created");
            entity.Property(e => e.LastUpdated).HasColumnName("lastUpdated");
        });

        // Configure OrcamentoView (view mapping)
        modelBuilder.Entity<OrcamentoView>(entity =>
        {
            entity.HasNoKey();
            entity.ToView("view_orcamento");

            entity.Property(e => e.IdOrcamento).HasColumnName("idOrcamento");
            entity.Property(e => e.IdLoja).HasColumnName("idLoja");
            entity.Property(e => e.IdUsuario).HasColumnName("idUsuario");
            entity.Property(e => e.IdUsuarioConsultor).HasColumnName("idUsuarioConsultor");
            entity.Property(e => e.Status).HasColumnName("status");
            entity.Property(e => e.DiasRestantes).HasColumnName("diasRestantes");
            entity.Property(e => e.Vendedor).HasColumnName("vendedor");
            entity.Property(e => e.Consultor).HasColumnName("consultor");
            entity.Property(e => e.Cliente).HasColumnName("cliente");
            entity.Property(e => e.Profissional).HasColumnName("profissional");
            entity.Property(e => e.Tel).HasColumnName("tel");
            entity.Property(e => e.TelCel).HasColumnName("telCel");
            entity.Property(e => e.TelProf).HasColumnName("telProf");
            entity.Property(e => e.Data).HasColumnName("data");
            entity.Property(e => e.Data2).HasColumnName("data2");
            entity.Property(e => e.Total).HasColumnName("total");
            entity.Property(e => e.IdFollowup).HasColumnName("idFollowup");
            entity.Property(e => e.DataFollowup).HasColumnName("dataFollowup");
            entity.Property(e => e.DataProxFollowup).HasColumnName("dataProxFollowup");
            entity.Property(e => e.Observacao).HasColumnName("observacao");
            entity.Property(e => e.Semaforo).HasColumnName("semaforo");
            entity.Property(e => e.Fornecedores).HasColumnName("fornecedores");
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
    public string? SenhaUsoUnico { get; set; }
    public decimal? ValorMinimoFrete { get; set; }
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

/// <summary>
/// Maintenance entity - system maintenance flag
/// </summary>
public class Maintenance
{
    public int Id { get; set; }
    public DateTime? LastInvalidated { get; set; }
    public DateTime? UltimaConsultaNSU { get; set; }
    public bool EmManutencao { get; set; }
    public DateTime Created { get; set; }
    public DateTime LastUpdated { get; set; }
}

/// <summary>
/// OrcamentoView - maps to view_orcamento database view
/// </summary>
public class OrcamentoView
{
    public string IdOrcamento { get; set; } = string.Empty;
    public int IdLoja { get; set; }
    public int IdUsuario { get; set; }
    public int? IdUsuarioConsultor { get; set; }
    public string? Status { get; set; }
    public int? DiasRestantes { get; set; }
    public string? Vendedor { get; set; }
    public string? Consultor { get; set; }
    public string? Cliente { get; set; }
    public string? Profissional { get; set; }
    public string? Tel { get; set; }
    public string? TelCel { get; set; }
    public string? TelProf { get; set; }
    public DateTime Data { get; set; }
    public string? Data2 { get; set; }
    public decimal Total { get; set; }
    public int? IdFollowup { get; set; }
    public DateTime? DataFollowup { get; set; }
    public DateTime? DataProxFollowup { get; set; }
    public string? Observacao { get; set; }
    public int? Semaforo { get; set; }
    public string? Fornecedores { get; set; }
}
