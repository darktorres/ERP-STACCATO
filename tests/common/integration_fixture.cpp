#include "integration_fixture.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QtTest>

#include <stdexcept>

namespace integration {

namespace {

QString readEnv(const char *name, const QString &fallback) {
  const QByteArray v = qgetenv(name);
  return v.isEmpty() ? fallback : QString::fromLocal8Bit(v);
}

// Walk up from the test binary's directory looking for the marker files
// that identify the repo root (initdb.sql + Loja.pro side by side).
// Returns the absolute repo root path, or an empty string if not found.
QString findRepoRoot() {
  QDir d(QCoreApplication::applicationDirPath());
  for (int hop = 0; hop < 8; ++hop) {
    if (d.exists(QStringLiteral("initdb.sql")) and d.exists(QStringLiteral("Loja.pro"))) { return d.absolutePath(); }
    if (not d.cdUp()) { break; }
  }
  return {};
}

// Locate the mysql command-line client. Probes env var, then PATH, then
// well-known Windows install dirs.
QString findMysqlBin() {
  if (const QByteArray env = qgetenv("STACCATO_TEST_MYSQL_BIN"); not env.isEmpty()) { return QString::fromLocal8Bit(env); }

  if (const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("mysql")); not fromPath.isEmpty()) { return fromPath; }

  for (const auto &candidate : {
           QStringLiteral("C:/Program Files/MySQL/MySQL Server 8.4/bin/mysql.exe"),
           QStringLiteral("C:/Program Files/MySQL/MySQL Server 8.0/bin/mysql.exe"),
           QStringLiteral("C:/Program Files/MySQL/MySQL Server 5.7/bin/mysql.exe"),
       }) {
    if (QFile::exists(candidate)) { return candidate; }
  }

  return {};
}

// Open a connection to the server's `mysql` meta-database (used only to
// CREATE the test schema if needed).
QSqlDatabase openAdminConnection(const ConnectionInfo &info) {
  const QString name = QStringLiteral("tier2_admin");

  if (QSqlDatabase::contains(name)) { QSqlDatabase::removeDatabase(name); }

  auto db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), name);
  db.setHostName(info.host);
  db.setPort(info.port);
  db.setUserName(info.user);
  db.setPassword(info.pass);
  db.setDatabaseName(QStringLiteral("mysql"));
  db.setConnectOptions(QStringLiteral("MYSQL_OPT_CONNECT_TIMEOUT=3"));

  db.open();
  return db;
}

bool schemaExists(QSqlDatabase &adminDb, const QString &dbName) {
  QSqlQuery q(adminDb);
  q.prepare(QStringLiteral("SHOW DATABASES LIKE :n"));
  q.bindValue(QStringLiteral(":n"), dbName);

  if (not q.exec()) { return false; }

  return q.next();
}

bool tablesLoaded(QSqlDatabase &db) {
  // `loja` is a top-level domain table created near the top of initdb.sql;
  // its absence is a reliable signal that the schema needs loading.
  QSqlQuery q(db);
  if (not q.exec(QStringLiteral("SHOW TABLES LIKE 'loja'"))) { return false; }
  return q.next();
}

// `initdb.sql` is a dump of the production schema with ~1170 hardcoded
// `` `staccato` `` references (USE statements + qualified table names). It
// can't be loaded into a different DB name directly. We stream it through
// a rewrite that swaps every `` `staccato` `` for `` `<info.dbName>` ``
// into a temp file, then feed THAT to mysql.exe.
QString rewriteInitDbToTempFile(const QString &initSqlPath, const QString &dbName) {
  QFile orig(initSqlPath);
  if (not orig.open(QFile::ReadOnly)) { throw std::runtime_error(QString::fromLatin1("Cannot read %1: %2").arg(initSqlPath, orig.errorString()).toStdString()); }

  // 410 KB file — comfortably small to slurp.
  QString sql = QString::fromUtf8(orig.readAll());
  sql.replace(QStringLiteral("`staccato`"), QStringLiteral("`%1`").arg(dbName));

  auto *temp = new QTemporaryFile(QDir::tempPath() + QStringLiteral("/staccato-initdb-XXXXXX.sql"));
  temp->setAutoRemove(false); // keep alive across QProcess; caller deletes
  if (not temp->open()) {
    const QString err = temp->errorString();
    delete temp;
    throw std::runtime_error(QString::fromLatin1("Cannot create temp file: %1").arg(err).toStdString());
  }

  temp->write(sql.toUtf8());
  const QString path = temp->fileName();
  temp->close();
  delete temp;
  return path;
}

// Run a .sql script through the mysql client, after rewriting `staccato`
// references to the test DB name. Used for both initdb.sql (schema) and
// fixtures.sql (canonical seed).
void runSqlScript(const ConnectionInfo &info, const QString &sqlPath) {
  const QString mysqlBin = findMysqlBin();
  if (mysqlBin.isEmpty()) {
    throw std::runtime_error("mysql client not found. Set STACCATO_TEST_MYSQL_BIN to its path or "
                             "ensure mysql.exe is on PATH.");
  }

  const QString rewrittenPath = rewriteInitDbToTempFile(sqlPath, info.dbName);
  // RAII delete on the rewritten temp file regardless of how we exit.
  struct TempCleanup {
    QString path;
    ~TempCleanup() { QFile::remove(path); }
  } cleanup{rewrittenPath};

  QProcess p;
  p.setStandardInputFile(rewrittenPath);

  QStringList args;
  args << QStringLiteral("--host=") + info.host;
  args << QStringLiteral("--port=") + QString::number(info.port);
  args << QStringLiteral("--user=") + info.user;
  if (not info.pass.isEmpty()) { args << QStringLiteral("--password=") + info.pass; }
  args << QStringLiteral("--default-character-set=utf8mb4");
  args << info.dbName;

  p.start(mysqlBin, args);
  if (not p.waitForFinished(120000)) {
    throw std::runtime_error(QString::fromLatin1("mysql client timed out loading initdb.sql: %1").arg(QString::fromLocal8Bit(p.readAllStandardError())).toStdString());
  }

  if (p.exitCode() != 0) {
    const QString stderr_ = QString::fromLocal8Bit(p.readAllStandardError());
    const QString stdout_ = QString::fromLocal8Bit(p.readAllStandardOutput());
    throw std::runtime_error(QString::fromLatin1("mysql client failed loading %1 (exit %2).\nstderr: %3\nstdout: %4")
                                 .arg(QFileInfo(sqlPath).fileName())
                                 .arg(p.exitCode())
                                 .arg(stderr_, stdout_)
                                 .toStdString());
  }
}

void ensureSchemaLoaded(const ConnectionInfo &info) {
  // Use admin connection in a tight scope so it's closed before we
  // removeDatabase (otherwise Qt warns "connection still in use").
  {
    auto adminDb = openAdminConnection(info);
    if (not adminDb.isOpen()) {
      QString hint = QStringLiteral("Set STACCATO_TEST_DB_USER/PASS env vars (see tests/README.md).");
      // The vendored libmysql.dll ships without caching_sha2_password.dll, so
      // MySQL 8+ accounts using that auth method fail here. Detect and steer.
      if (adminDb.lastError().text().contains(QStringLiteral("caching_sha2_password"), Qt::CaseInsensitive)) {
        hint = QStringLiteral("This project uses mysql_native_password (not caching_sha2_password). "
                              "Create a dedicated user via "
                              "`CREATE USER 'staccato_test'@'localhost' IDENTIFIED WITH "
                              "mysql_native_password BY '…'` and set "
                              "STACCATO_TEST_DB_USER/PASS to point at it. "
                              "See tests/README.md Tier 2 section.");
      }
      throw std::runtime_error(QString::fromLatin1("Cannot connect to MySQL server (%1@%2:%3): %4. %5")
                                   .arg(info.user, info.host)
                                   .arg(info.port)
                                   .arg(adminDb.lastError().text(), hint)
                                   .toStdString());
    }

    if (not schemaExists(adminDb, info.dbName)) {
      QSqlQuery q(adminDb);
      if (not q.exec(QStringLiteral("CREATE DATABASE `%1` CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci").arg(info.dbName))) {
        throw std::runtime_error(QString::fromLatin1("CREATE DATABASE %1 failed: %2").arg(info.dbName, q.lastError().text()).toStdString());
      }
      qInfo("[tier2] created database '%s'", qPrintable(info.dbName));
    }

    adminDb.close();
  }
  QSqlDatabase::removeDatabase(QStringLiteral("tier2_admin"));

  // Probe whether tables are already loaded. We do this in a nested scope
  // so the QSqlDatabase value goes out of scope BEFORE we call
  // removeDatabase (otherwise Qt warns "connection still in use").
  bool needsLoad = false;
  {
    const QString probeName = QStringLiteral("tier2_probe");
    if (QSqlDatabase::contains(probeName)) { QSqlDatabase::removeDatabase(probeName); }
    {
      auto probe = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), probeName);
      probe.setHostName(info.host);
      probe.setPort(info.port);
      probe.setUserName(info.user);
      probe.setPassword(info.pass);
      probe.setDatabaseName(info.dbName);
      probe.setConnectOptions(QStringLiteral("MYSQL_OPT_CONNECT_TIMEOUT=3"));

      if (not probe.open()) { throw std::runtime_error(QString::fromLatin1("Cannot open '%1' after CREATE: %2").arg(info.dbName, probe.lastError().text()).toStdString()); }

      needsLoad = not tablesLoaded(probe);
      probe.close();
    }
    QSqlDatabase::removeDatabase(probeName);
  }

  if (needsLoad) {
    const QString repoRoot = findRepoRoot();
    if (repoRoot.isEmpty()) {
      throw std::runtime_error("Cannot locate repo root (initdb.sql + Loja.pro). "
                               "Run tier2 tests from inside the repo tree.");
    }

    const QString initSql = repoRoot + QStringLiteral("/initdb.sql");
    qInfo("[tier2] loading %s into '%s' (this takes ~10s)…", qPrintable(initSql), qPrintable(info.dbName));

    runSqlScript(info, initSql);

    qInfo("[tier2] schema loaded");

    // After the initial schema load, also apply the canonical seed
    // (fixtures.sql). We only do this when we just loaded the schema —
    // subsequent runs (which find `loja` already present) skip both.
    const QString fixturesSql = repoRoot + QStringLiteral("/tests/fixtures/fixtures.sql");
    if (QFile::exists(fixturesSql)) {
      qInfo("[tier2] loading fixtures from %s", qPrintable(fixturesSql));
      runSqlScript(info, fixturesSql);
      qInfo("[tier2] fixtures loaded");
    }
  }
}

} // namespace

ConnectionInfo readEnvConnectionInfo() {
  ConnectionInfo info;
  info.host = readEnv("STACCATO_TEST_DB_HOST", QStringLiteral("127.0.0.1"));
  info.port = readEnv("STACCATO_TEST_DB_PORT", QStringLiteral("3306")).toInt();
  info.user = readEnv("STACCATO_TEST_DB_USER", QStringLiteral("root"));
  info.pass = readEnv("STACCATO_TEST_DB_PASS", QString());
  info.dbName = readEnv("STACCATO_TEST_DB_NAME", QStringLiteral("staccato_test"));

  if (info.dbName == QStringLiteral("staccato") or info.dbName == QStringLiteral("staccato_staging")) {
    throw std::runtime_error(QString::fromLatin1("Refusing to use production schema '%1' for Tier 2 tests. "
                                                 "Override STACCATO_TEST_DB_NAME with a non-production name.")
                                 .arg(info.dbName)
                                 .toStdString());
  }

  return info;
}

QSqlDatabase openTestConnection(const QString &connectionName) {
  const ConnectionInfo info = readEnvConnectionInfo();

  ensureSchemaLoaded(info);

  if (QSqlDatabase::contains(connectionName)) { QSqlDatabase::removeDatabase(connectionName); }

  auto db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), connectionName);
  db.setHostName(info.host);
  db.setPort(info.port);
  db.setUserName(info.user);
  db.setPassword(info.pass);
  db.setDatabaseName(info.dbName);
  db.setConnectOptions(QStringLiteral("MYSQL_OPT_CONNECT_TIMEOUT=3"));

  db.open();
  return db;
}

void closeTestConnection(const QString &connectionName) {
  if (QSqlDatabase::contains(connectionName)) {
    {
      auto db = QSqlDatabase::database(connectionName, false);
      if (db.isOpen()) { db.close(); }
    }
    QSqlDatabase::removeDatabase(connectionName);
  }
}

// -----------------------------------------------------------------------------
// IntegrationFixture
// -----------------------------------------------------------------------------

void IntegrationFixture::initTestCase() {
  try {
    db = openTestConnection();
  } catch (const std::exception &e) { QSKIP(e.what()); }

  if (not db.isOpen()) { QSKIP(qPrintable(QStringLiteral("Cannot open test DB: %1").arg(db.lastError().text()))); }
}

void IntegrationFixture::cleanupTestCase() {
  // Release the fixture's handle BEFORE removeDatabase, otherwise Qt warns
  // "connection still in use".
  if (db.isOpen()) { db.close(); }
  db = QSqlDatabase();
  closeTestConnection();
}

void IntegrationFixture::init() {
  if (not db.isOpen()) { QSKIP("DB connection not open"); }

  // QSqlDatabase::transaction() returns false here even on InnoDB because the
  // bundled libmysql.dll doesn't advertise the CLIENT_TRANSACTIONS capability
  // bit, so Qt's hasFeature(Transactions) gates the call. Bypass via raw SQL.
  QSqlQuery q(db);
  QVERIFY2(q.exec(QStringLiteral("START TRANSACTION")), qPrintable(q.lastError().text()));
}

void IntegrationFixture::cleanup() {
  if (not db.isOpen()) { return; }
  QSqlQuery q(db);
  q.exec(QStringLiteral("ROLLBACK"));
}

} // namespace integration
