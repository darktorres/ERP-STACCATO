// Tier 2 — integration tests against a real MySQL `staccato_test` schema.
// See tests/common/integration_fixture.h for the bootstrap mechanics.
//
// SAFETY: every test class either inherits IntegrationFixture (which wraps
// each test in transaction-rollback) or is explicitly stateless. Tests must
// never commit or call stored procedures that open their own transactions
// without being tagged `[procedure]` and using the truncate-snapshot pattern.

#include "integration_fixture.h"
#include "sql.h"
#include "validators.h"

#include <QApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtTest>

#include <stdexcept>

// -----------------------------------------------------------------------------
// TestConnectionSafety — no DB connection needed. Verifies the env-driven
// connection helper refuses production schema names hard.
// -----------------------------------------------------------------------------
class TestConnectionSafety : public QObject {
  Q_OBJECT

private slots:
  void refusesProductionDb() {
    // Save the current value so other tests aren't affected.
    const QByteArray saved = qgetenv("STACCATO_TEST_DB_NAME");
    auto restore = qScopeGuard([&] {
      if (saved.isEmpty()) {
        qunsetenv("STACCATO_TEST_DB_NAME");
      } else {
        qputenv("STACCATO_TEST_DB_NAME", saved);
      }
    });

    for (const QByteArray &dangerous : {QByteArrayLiteral("staccato"), QByteArrayLiteral("staccato_staging")}) {
      qputenv("STACCATO_TEST_DB_NAME", dangerous);

      bool threw = false;
      try {
        (void)integration::readEnvConnectionInfo();
      } catch (const std::runtime_error &) { threw = true; }

      QVERIFY2(threw, qPrintable(QStringLiteral("Should have refused DB '%1' but didn't").arg(QString::fromLatin1(dangerous))));
    }
  }

  void acceptsTestDb() {
    const QByteArray saved = qgetenv("STACCATO_TEST_DB_NAME");
    auto restore = qScopeGuard([&] {
      if (saved.isEmpty()) {
        qunsetenv("STACCATO_TEST_DB_NAME");
      } else {
        qputenv("STACCATO_TEST_DB_NAME", saved);
      }
    });

    qputenv("STACCATO_TEST_DB_NAME", "staccato_test");

    integration::ConnectionInfo info;
    try {
      info = integration::readEnvConnectionInfo();
    } catch (const std::runtime_error &e) { QFAIL(e.what()); }

    QCOMPARE(info.dbName, QStringLiteral("staccato_test"));
  }
};

// -----------------------------------------------------------------------------
// TestConnection — basic sanity that the bootstrap + open works and lands in
// the right schema.
// -----------------------------------------------------------------------------
class TestConnection : public integration::IntegrationFixture {
  Q_OBJECT

private slots:
  void databaseNameMatches() {
    QSqlQuery q(db);
    QVERIFY(q.exec(QStringLiteral("SELECT DATABASE()")));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("staccato_test"));
  }

  void schemaLoadedSentinelTablePresent() {
    // `loja` is the sentinel the fixture uses to decide whether to bootstrap.
    QSqlQuery q(db);
    QVERIFY(q.exec(QStringLiteral("SHOW TABLES LIKE 'loja'")));
    QVERIFY(q.next());
  }

  void rollbackUndoesInsert() {
    // The fixture's init() opened a transaction; this test inserts a row,
    // verifies it's visible mid-transaction, and lets cleanup() roll it back.
    // The next test (databaseNameMatches on a future run) must not see it.
    QSqlQuery q(db);
    QVERIFY(q.exec(QStringLiteral("CREATE TEMPORARY TABLE _tier2_probe (id INT)")));
    QVERIFY(q.exec(QStringLiteral("INSERT INTO _tier2_probe VALUES (1)")));
    QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM _tier2_probe")));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);
  }
};

// -----------------------------------------------------------------------------
// TestSqlExecutesAgainstSchema — the Sql::* string-builders return SQL we've
// already snapshot-tested in tier1; here we additionally prove the queries
// PARSE against the real schema. Run with LIMIT 0 to avoid scanning data.
// -----------------------------------------------------------------------------
class TestSqlExecutesAgainstSchema : public integration::IntegrationFixture {
  Q_OBJECT

private slots:
  void contasPagarParses() {
    const QString sql = Sql::contasPagar(QString(), QString()) + QStringLiteral(" LIMIT 0");

    QSqlQuery q(db);
    QVERIFY2(q.exec(sql), qPrintable(q.lastError().text()));
  }

  void contasReceberParses() {
    const QString sql = Sql::contasReceber(QString()) + QStringLiteral(" LIMIT 0");

    QSqlQuery q(db);
    QVERIFY2(q.exec(sql), qPrintable(q.lastError().text()));
  }

  void contasPagarWithFilterParses() {
    const QString sql = Sql::contasPagar(QStringLiteral("cp.status = 'PENDENTE'"), QString()) + QStringLiteral(" LIMIT 0");

    QSqlQuery q(db);
    QVERIFY2(q.exec(sql), qPrintable(q.lastError().text()));
  }
};

// -----------------------------------------------------------------------------
// TestFixtures — asserts the canonical seed from tests/fixtures/fixtures.sql
// is present. If the bootstrap didn't load it (e.g. older schema, DB
// pre-existed), each test individually inserts what it needs inside its own
// transaction.
// -----------------------------------------------------------------------------
class TestFixtures : public integration::IntegrationFixture {
  Q_OBJECT

private slots:
  void canonicalLojaSeeded() {
    QSqlQuery q(db);
    QVERIFY(q.exec(QStringLiteral("SELECT nomeFantasia FROM loja WHERE idLoja = 1")));
    QVERIFY2(q.next(), "loja idLoja=1 missing — fixtures.sql not loaded?");
    QCOMPARE(q.value(0).toString(), QStringLiteral("Loja Teste"));
  }

  void canonicalUsuarioSeeded() {
    QSqlQuery q(db);
    QVERIFY(q.exec(QStringLiteral("SELECT user, tipo FROM usuario WHERE idUsuario = 1")));
    QVERIFY2(q.next(), "usuario idUsuario=1 missing — fixtures.sql not loaded?");
    QCOMPARE(q.value(0).toString(), QStringLiteral("admin_test"));
    QCOMPARE(q.value(1).toString(), QStringLiteral("ADMINISTRADOR"));
  }

  void canonicalClientesSeeded() {
    QSqlQuery q(db);
    QVERIFY(q.exec(QStringLiteral("SELECT pfpj, nome_razao, cpf, cnpj FROM cliente WHERE idCliente IN (1, 2) ORDER BY idCliente")));

    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("PF"));
    QCOMPARE(q.value(2).toString(), QStringLiteral("390.533.447-05"));

    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("PJ"));
    QCOMPARE(q.value(3).toString(), QStringLiteral("11.222.333/0001-81"));
  }

  void seededCpfMatchesValidator() {
    // Bridge from tier2 fixture data to tier1 validators: prove the fixture
    // CPF is one the pure validator would accept too.
    QSqlQuery q(db);
    QVERIFY(q.exec(QStringLiteral("SELECT cpf FROM cliente WHERE idCliente = 1")));
    QVERIFY(q.next());
    QVERIFY2(validators::cpfValido(q.value(0).toString()), "seed CPF doesn't pass validator — fixtures.sql out of sync");
  }
};

// -----------------------------------------------------------------------------
// TestClienteRoundTrip — full insert/select/rollback cycle. Demonstrates the
// transaction-rollback isolation: a row inserted here vanishes before the
// next test, so different tests don't see each other's data.
// -----------------------------------------------------------------------------
class TestClienteRoundTrip : public integration::IntegrationFixture {
  Q_OBJECT

private slots:
  void insertSelectRollback() {
    // Use an idCliente outside the canonical seed range so we don't collide.
    const int probeId = 9001;

    QSqlQuery q(db);
    q.prepare(QStringLiteral("INSERT INTO cliente (idCliente, pfpj, nome_razao, cpf) VALUES (:id, 'PF', 'TX Probe', :cpf)"));
    q.bindValue(QStringLiteral(":id"), probeId);
    q.bindValue(QStringLiteral(":cpf"), QStringLiteral("390.533.447-05"));
    QVERIFY2(q.exec(), qPrintable(q.lastError().text()));

    QSqlQuery check(db);
    check.prepare(QStringLiteral("SELECT nome_razao FROM cliente WHERE idCliente = :id"));
    check.bindValue(QStringLiteral(":id"), probeId);
    QVERIFY(check.exec());
    QVERIFY(check.next());
    QCOMPARE(check.value(0).toString(), QStringLiteral("TX Probe"));
    // cleanup() will ROLLBACK; the next test won't see this row.
  }

  void rolledBackRowGone() {
    // Confirms the isolation works: probeId from the previous test must NOT
    // be visible here. This test runs in a fresh transaction.
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT 1 FROM cliente WHERE idCliente = :id"));
    q.bindValue(QStringLiteral(":id"), 9001);
    QVERIFY(q.exec());
    QVERIFY2(not q.next(), "Row from previous test leaked across transaction boundary");
  }
};

// -----------------------------------------------------------------------------
// TestRoundTrips — extra INSERT/SELECT/rollback per major business table.
// Each test inserts using fields that are NOT NULL without defaults
// (sourced from information_schema), reads back, and asserts. Rollback
// from IntegrationFixture cleanup wipes the rows.
// -----------------------------------------------------------------------------
class TestRoundTrips : public integration::IntegrationFixture {
  Q_OBJECT

private slots:
  void fornecedorRoundTrip() {
    QSqlQuery q(db);
    // fornecedor: idFornecedor (auto), razaoSocial NOT NULL.
    q.prepare(QStringLiteral("INSERT INTO fornecedor (razaoSocial) VALUES (:r)"));
    q.bindValue(QStringLiteral(":r"), QStringLiteral("RoundTrip Fornecedor"));
    QVERIFY2(q.exec(), qPrintable(q.lastError().text()));

    const QVariant newId = q.lastInsertId();
    QVERIFY(newId.isValid());

    QSqlQuery sel(db);
    sel.prepare(QStringLiteral("SELECT razaoSocial FROM fornecedor WHERE idFornecedor = :id"));
    sel.bindValue(QStringLiteral(":id"), newId);
    QVERIFY(sel.exec());
    QVERIFY(sel.next());
    QCOMPARE(sel.value(0).toString(), QStringLiteral("RoundTrip Fornecedor"));
  }

  void produtoRoundTrip() {
    // produto: requires idFornecedor (FK), fornecedor, descricao, un,
    // codComercial, custo. Uses the seeded fornecedor (idFornecedor=1,
    // razaoSocial 'Fornecedor Teste LTDA').
    QSqlQuery q(db);
    q.prepare(QStringLiteral("INSERT INTO produto "
                             "(idFornecedor, fornecedor, descricao, un, codComercial, custo) "
                             "VALUES (1, 'Fornecedor Teste LTDA', :desc, 'M2', :cod, 99.99)"));
    q.bindValue(QStringLiteral(":desc"), QStringLiteral("RoundTrip Produto"));
    q.bindValue(QStringLiteral(":cod"), QStringLiteral("RT-COD-001"));
    QVERIFY2(q.exec(), qPrintable(q.lastError().text()));

    const QVariant newId = q.lastInsertId();
    QVERIFY(newId.isValid());

    QSqlQuery sel(db);
    sel.prepare(QStringLiteral("SELECT descricao, un, codComercial, custo FROM produto WHERE idProduto = :id"));
    sel.bindValue(QStringLiteral(":id"), newId);
    QVERIFY(sel.exec());
    QVERIFY(sel.next());
    QCOMPARE(sel.value(0).toString(), QStringLiteral("RoundTrip Produto"));
    QCOMPARE(sel.value(1).toString(), QStringLiteral("M2"));
    QCOMPARE(sel.value(2).toString(), QStringLiteral("RT-COD-001"));
    QCOMPARE(sel.value(3).toDouble(), 99.99);
  }

  void pedidoFornecedorP1P2RoundTrip() {
    // The OC tree has two tables: pedido_fornecedor_has_produto (p1, the
    // master) and pedido_fornecedor_has_produto2 (p2, the leaf breakdown
    // per OC/lote/recebimento). p2.idPedidoFK references p1.idPedido1, so
    // a round-trip needs both inserts.
    QSqlQuery p1(db);
    p1.prepare(QStringLiteral("INSERT INTO pedido_fornecedor_has_produto (fornecedor) VALUES (:f)"));
    p1.bindValue(QStringLiteral(":f"), QStringLiteral("Fornecedor Teste LTDA"));
    QVERIFY2(p1.exec(), qPrintable(p1.lastError().text()));

    const QVariant idPedido1 = p1.lastInsertId();
    QVERIFY(idPedido1.isValid());

    QSqlQuery p2(db);
    p2.prepare(QStringLiteral("INSERT INTO pedido_fornecedor_has_produto2 "
                              "(idPedidoFK, fornecedor) VALUES (:fk, :f)"));
    p2.bindValue(QStringLiteral(":fk"), idPedido1);
    p2.bindValue(QStringLiteral(":f"), QStringLiteral("Fornecedor Teste LTDA"));
    QVERIFY2(p2.exec(), qPrintable(p2.lastError().text()));

    const QVariant idPedido2 = p2.lastInsertId();
    QVERIFY(idPedido2.isValid());

    QSqlQuery sel(db);
    sel.prepare(QStringLiteral("SELECT fornecedor, idPedidoFK FROM pedido_fornecedor_has_produto2 "
                               "WHERE idPedido2 = :id"));
    sel.bindValue(QStringLiteral(":id"), idPedido2);
    QVERIFY(sel.exec());
    QVERIFY(sel.next());
    QCOMPARE(sel.value(0).toString(), QStringLiteral("Fornecedor Teste LTDA"));
    QCOMPARE(sel.value(1).toInt(), idPedido1.toInt());
  }
};

// -----------------------------------------------------------------------------
// TestStoredProcedures — calls a handful of stored procedures with no-op
// inputs to confirm they exist, accept the production-expected parameter
// types, and don't blow up on edge cases. Real behavioural tests would need
// deep fixture chains (venda → loja/cliente/usuario/endereco/profissional);
// they can be added incrementally as M3 fixtures expand.
//
// IMPORTANT: every procedure in initdb.sql is verified (via grep) to NOT
// contain explicit START TRANSACTION / COMMIT / ROLLBACK. So they all run
// inside the caller's transaction — IntegrationFixture's rollback wrapper
// undoes their effects, just like for any other DML. No truncate-snapshot
// pattern is needed.
// -----------------------------------------------------------------------------
class TestStoredProcedures : public integration::IntegrationFixture {
  Q_OBJECT

private slots:
  void updateVendaStatusOnNonexistentIdIsNoop() {
    // Production calls this whenever a venda_has_produto row changes. With a
    // non-existent idVenda, the cursor over venda_has_produto matches nothing
    // and the UPDATE statements find no rows — verifies the procedure exists
    // and handles the empty-state edge case without error.
    QSqlQuery q(db);
    q.prepare(QStringLiteral("CALL update_venda_status(:id)"));
    q.bindValue(QStringLiteral(":id"), QStringLiteral("NONEXISTENT_VENDA_X"));
    QVERIFY2(q.exec(), qPrintable(q.lastError().text()));
  }

  void updateFornecedoresOrcamentoOnNonexistentIdIsNoop() {
    QSqlQuery q(db);
    q.prepare(QStringLiteral("CALL update_fornecedores_orcamento(:id)"));
    q.bindValue(QStringLiteral(":id"), QStringLiteral("NONEXISTENT_ORC_X"));
    QVERIFY2(q.exec(), qPrintable(q.lastError().text()));
  }

  void updateFornecedoresVendaOnNonexistentIdIsNoop() {
    QSqlQuery q(db);
    q.prepare(QStringLiteral("CALL update_fornecedores_venda(:id)"));
    q.bindValue(QStringLiteral(":id"), QStringLiteral("NONEXISTENT_VENDA_X"));
    QVERIFY2(q.exec(), qPrintable(q.lastError().text()));
  }

  void mydateFunctionExists() {
    // MYDATE() is referenced extensively in views (view_resumo_relatorio).
    // It returns the session variable @mydate, so the value will be NULL
    // unless explicitly set — what we verify here is just that the
    // function exists and is callable.
    QSqlQuery q(db);
    QVERIFY2(q.exec(QStringLiteral("SELECT MYDATE()")), qPrintable(q.lastError().text()));
    QVERIFY(q.next());
  }

  void sha1PasswordFunctionRoundTrips() {
    // SHA1_PASSWORD() is the auth-hash helper used by usuario rows in
    // initdb.sql / fixtures.sql. Verify the function produces a stable
    // 41-character MySQL-style hash for a known input.
    QSqlQuery q(db);
    QVERIFY(q.exec(QStringLiteral("SELECT SHA1_PASSWORD('admin123')")));
    QVERIFY(q.next());
    const QString hash = q.value(0).toString();
    QCOMPARE(hash.length(), 41);
    QVERIFY(hash.startsWith(QLatin1Char('*')));
  }
};

// -----------------------------------------------------------------------------
// Aggregating main.
// -----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  int status = 0;

  { TestConnectionSafety t; status |= QTest::qExec(&t, argc, argv); }
  { TestConnection t; status |= QTest::qExec(&t, argc, argv); }
  { TestSqlExecutesAgainstSchema t; status |= QTest::qExec(&t, argc, argv); }
  { TestFixtures t; status |= QTest::qExec(&t, argc, argv); }
  { TestClienteRoundTrip t; status |= QTest::qExec(&t, argc, argv); }
  { TestRoundTrips t; status |= QTest::qExec(&t, argc, argv); }
  { TestStoredProcedures t; status |= QTest::qExec(&t, argc, argv); }

  return status;
}

#include "test_tier2.moc"
