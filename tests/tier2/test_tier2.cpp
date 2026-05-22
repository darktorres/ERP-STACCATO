// Tier 2 — integration tests against a real MySQL `staccato_test` schema.
// See tests/common/integration_fixture.h for the bootstrap mechanics.
//
// SAFETY: every test class either inherits IntegrationFixture (which wraps
// each test in transaction-rollback) or is explicitly stateless. Tests must
// never commit or call stored procedures that open their own transactions
// without being tagged `[procedure]` and using the truncate-snapshot pattern.

#include "integration_fixture.h"
#include "sql.h"

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
// Aggregating main.
// -----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  int status = 0;

  { TestConnectionSafety t; status |= QTest::qExec(&t, argc, argv); }
  { TestConnection t; status |= QTest::qExec(&t, argc, argv); }
  { TestSqlExecutesAgainstSchema t; status |= QTest::qExec(&t, argc, argv); }

  return status;
}

#include "test_tier2.moc"
