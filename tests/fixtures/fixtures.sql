-- Tier 2 canonical seed.
-- Loaded by tests/common/integration_fixture.cpp AFTER initdb.sql.
-- Same `staccato` → `<dbname>` rewrite applies, so unqualified table names
-- still resolve via the connection's default schema.
--
-- Keep this lean. Tests that need transient rows insert them inside their
-- own transaction (IntegrationFixture wraps each test in START
-- TRANSACTION / ROLLBACK), so seeded data is for canonical *shape* — the
-- IDs and CPF/CNPJ values are stable so tests can hardcode them.
--
-- Conventions:
--   * idLoja=1, idUsuario=1 — referenced by other tables if needed.
--   * idCliente=1 is a valid PF (CPF 390.533.447-05, the same value used
--     in tier1 validator tests).
--   * idCliente=2 is a valid PJ (CNPJ 11.222.333/0001-81).
--
-- This script is idempotent via `INSERT IGNORE`: re-running on an already
-- seeded schema is a no-op. Tests that want a clean slate per run should
-- DROP the schema first (the bootstrap will re-CREATE and reload).

INSERT IGNORE INTO `loja`
    (`idLoja`, `nomeFantasia`, `razaoSocial`, `tel`)
VALUES
    (1, 'Loja Teste',  'Loja Teste LTDA', '11999999999');

INSERT IGNORE INTO `usuario`
    (`idUsuario`, `idLoja`, `user`, `passwd`, `tipo`, `nome`)
VALUES
    (1, 1, 'admin_test', SHA1_PASSWORD('admin123'), 'ADMINISTRADOR', 'Test Admin');

INSERT IGNORE INTO `cliente`
    (`idCliente`, `pfpj`, `nome_razao`, `cpf`)
VALUES
    (1, 'PF', 'Cliente PF Teste', '390.533.447-05');

INSERT IGNORE INTO `cliente`
    (`idCliente`, `pfpj`, `nome_razao`, `cnpj`)
VALUES
    (2, 'PJ', 'Cliente PJ Teste LTDA', '11.222.333/0001-81');
