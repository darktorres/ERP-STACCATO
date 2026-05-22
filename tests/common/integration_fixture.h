#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>

// Tier 2 shared infrastructure: env-driven test DB connection with hard
// guards against connecting to production schemas, plus a QObject base
// class providing the standard transaction-rollback isolation between
// tests.
//
// SAFETY: by construction `openTestConnection()` refuses any database name
// matching `staccato` or `staccato_staging`. The default is `staccato_test`.
// Production passwords are never read (no `mysql.txt` lookup) — credentials
// come from env vars only:
//   STACCATO_TEST_DB_HOST   default 127.0.0.1
//   STACCATO_TEST_DB_PORT   default 3306
//   STACCATO_TEST_DB_USER   default root
//   STACCATO_TEST_DB_PASS   default empty
//   STACCATO_TEST_DB_NAME   default staccato_test
//   STACCATO_TEST_MYSQL_BIN override path to mysql.exe used for bootstrap
//
// BOOTSTRAP: if `staccato_test` doesn't exist on the server, it's created.
// If it exists but has no `loja` table, the connection helper executes
// `mysql … < initdb.sql` to load the full schema (216 tables + 106
// procedures/triggers/views). Idempotent: subsequent runs see the table
// and skip the load. The repo root is found by walking up from the .exe
// looking for `initdb.sql` + `Loja.pro`.

namespace integration {

struct ConnectionInfo {
  QString host;
  int port = 3306;
  QString user;
  QString pass;
  QString dbName;
};

// Read connection details from env vars (above). Validates the DB name is
// not a production schema; throws std::runtime_error if it is.
ConnectionInfo readEnvConnectionInfo();

// Open (or reuse) a named QSqlDatabase connection for Tier 2. Performs the
// bootstrap (CREATE DATABASE / load initdb.sql) if necessary. On any
// connection failure returns a non-open QSqlDatabase whose lastError()
// describes the problem; the caller (typically IntegrationFixture) is
// expected to QSKIP with a clear message.
QSqlDatabase openTestConnection(const QString &connectionName = QStringLiteral("tier2_test"));

// Close and remove the named connection. Safe to call even if the
// connection was never opened.
void closeTestConnection(const QString &connectionName = QStringLiteral("tier2_test"));

// Base class for Tier 2 test classes. Manages the connection lifecycle and
// wraps each test function in a transaction that rolls back on cleanup —
// every test starts from the same DB state, regardless of order.
//
// Procedure tests (those that CALL stored procedures opening their own
// transactions) bypass the rollback wrapper. See the per-test [procedure]
// tag convention in the plan file.
class IntegrationFixture : public QObject {
  Q_OBJECT

protected:
  QSqlDatabase db;

protected slots:
  void initTestCase();
  void cleanupTestCase();
  void init();
  void cleanup();
};

} // namespace integration
