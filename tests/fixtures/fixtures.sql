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

-- The schema has two password columns and two hash functions for historical
-- reasons: `passwd` (NOT NULL, hashed via SHA1_PASSWORD) is legacy, while
-- `password` (nullable, hashed via SHA_PASSWORD) is what User::login actually
-- checks against. Tier 3 tests authenticate via User::login('admin_test',
-- 'admin123'), so both columns need consistent test-credential values.
INSERT IGNORE INTO `usuario`
    (`idUsuario`, `idLoja`, `user`, `passwd`, `password`, `tipo`, `nome`)
VALUES
    (1, 1, 'admin_test', SHA1_PASSWORD('admin123'), SHA_PASSWORD('admin123'), 'ADMINISTRADOR', 'Test Admin');

INSERT IGNORE INTO `cliente`
    (`idCliente`, `pfpj`, `nome_razao`, `cpf`)
VALUES
    (1, 'PF', 'Cliente PF Teste', '390.533.447-05');

INSERT IGNORE INTO `cliente`
    (`idCliente`, `pfpj`, `nome_razao`, `cnpj`)
VALUES
    (2, 'PJ', 'Cliente PJ Teste LTDA', '11.222.333/0001-81');

INSERT IGNORE INTO `fornecedor`
    (`idFornecedor`, `razaoSocial`)
VALUES
    (1, 'Fornecedor Teste LTDA');

-- The endereço placeholder used by the Orcamento/Venda dialogs when the
-- client is picking up at the store. Production special-cases this id with
-- the literal text 'NÃO HÁ/RETIRA' (see orcamento.cpp:1261, venda.cpp:1525).
INSERT IGNORE INTO `cliente_has_endereco`
    (`idEndereco`, `idCliente`, `descricao`)
VALUES
    (1, NULL, 'NÃO HÁ/RETIRA');

-- A real address attached to the PF cliente seeded above. Reused by
-- venda below for both entrega and faturamento.
INSERT IGNORE INTO `cliente_has_endereco`
    (`idEndereco`, `idCliente`, `descricao`, `cep`, `logradouro`, `numero`, `bairro`, `cidade`, `uf`)
VALUES
    (2, 1, 'Endereço PF Teste', '01310-100', 'Av. Paulista', '1578', 'Bela Vista', 'São Paulo', 'SP');

-- The profissional placeholder used when a venda has no architect/designer.
-- idProfissional=1 is special-cased to "NÃO HÁ" throughout the code (see
-- registerdialog.cpp:38 — viewRegisterById refuses to load it).
INSERT IGNORE INTO `profissional`
    (`idProfissional`, `nome_razao`)
VALUES
    (1, 'NÃO HÁ');

-- A canonical venda row pulling together the seeded FKs. Used by the
-- TestVendaReadPath tier3 test to exercise Venda::viewRegisterById.
INSERT IGNORE INTO `venda`
    (`idVenda`, `idLoja`, `idUsuario`, `idCliente`, `idEnderecoEntrega`, `idEnderecoFaturamento`,
     `idProfissional`, `data`, `dataOrc`, `total`, `prazoEntrega`)
VALUES
    ('TEST-001', 1, 1, 1, 2, 2, 1, NOW(), NOW(), 0, 0);
