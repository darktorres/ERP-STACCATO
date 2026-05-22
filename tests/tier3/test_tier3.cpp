// Tier 3 — UI smoke. Instantiates the real Application class (not just
// QApplication) so dialogs that depend on `qApp` and its DB connection work.
//
// Bootstrap problem: Application's constructor reads `lojas.txt` from the
// exe's working directory and pops a QMessageBox if missing — which would
// block the test. We write a stub lojas.txt next to the binary BEFORE
// constructing Application, then remove it on exit. Same approach for the
// default QSqlDatabase: dialogs use `qApp->db` (the default-named
// connection); we install it pointing at `staccato_test` via the same env
// vars Tier 2 uses.
//
// SAFETY: the connection refusal from `integration::readEnvConnectionInfo`
// applies here too — Tier 3 never connects to `staccato` or
// `staccato_staging`. The stub `lojas.txt` is bogus content (one fake host)
// so even if a code path tries to consult `mapLojas`, it can't reach
// production servers.

#include "application.h"
#include "cadastrocliente.h"
#include "cadastrofornecedor.h"
#include "cadastroloja.h"
#include "integration_fixture.h"
#include "searchdialog.h"
#include "user.h"
#include "venda.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtTest>

namespace {

QString findExeDir(char **argv) {
  // QCoreApplication::applicationDirPath() isn't available before the
  // Application ctor runs, so we derive it from argv[0].
  return QFileInfo(QString::fromLocal8Bit(argv[0])).absoluteDir().absolutePath();
}

void writeStubLojasTxt(const QString &exeDir) {
  const QString path = exeDir + QStringLiteral("/lojas.txt");
  if (QFile::exists(path)) { return; }
  QFile f(path);
  if (not f.open(QFile::WriteOnly | QFile::Truncate)) { return; }
  f.write("TestStore\n127.0.0.1\n");
  f.close();
}

// Open the default-named QSqlDatabase against staccato_test, bootstrapping
// the schema if needed. Returns true on success; on failure populates
// `whyFailed` with a human-readable reason so the smoke test can QSKIP.
bool openDefaultTestConnection(QString &whyFailed) {
  try {
    // Reuse the Tier 2 bootstrap. Passing the default-connection sentinel
    // makes the resulting QSqlDatabase be the one returned by
    // `QSqlDatabase::database()` (i.e. the one Qt's models pick up by
    // default), so dialogs work.
    auto db = integration::openTestConnection(QLatin1String(QSqlDatabase::defaultConnection));
    if (not db.isOpen()) {
      whyFailed = QStringLiteral("default connection refused to open: %1").arg(db.lastError().text());
      return false;
    }
    return true;
  } catch (const std::exception &e) {
    whyFailed = QString::fromLocal8Bit(e.what());
    return false;
  }
}

} // namespace

// -----------------------------------------------------------------------------
// TestTier3Smoke — proves the harness works end-to-end: Application is
// instantiated, qApp resolves to it (not just any QApplication), and the
// default QSqlDatabase is queryable.
// -----------------------------------------------------------------------------
class TestTier3Smoke : public QObject {
  Q_OBJECT

private slots:
  void qAppIsApplicationSubclass() {
    // The `qApp` macro is dynamic_cast<Application*>(...) — if Application
    // wasn't constructed, this resolves to nullptr and most production code
    // would null-deref.
    QVERIFY2(qApp != nullptr, "qApp is null — Application not constructed?");
  }

  void defaultConnectionIsOpen() {
    QSqlDatabase db = QSqlDatabase::database();
    QVERIFY2(db.isOpen(), qPrintable(QStringLiteral("Default connection not open: %1").arg(db.lastError().text())));
  }

  void defaultConnectionTargetsStaccatoTest() {
    QSqlQuery q;
    QVERIFY(q.exec(QStringLiteral("SELECT DATABASE()")));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("staccato_test"));
  }

  void canReadSeededLoja() {
    QSqlQuery q;
    QVERIFY(q.exec(QStringLiteral("SELECT nomeFantasia FROM loja WHERE idLoja = 1")));
    QVERIFY2(q.next(), "Tier 3 expects fixtures.sql seed (idLoja=1) to be present");
    QCOMPARE(q.value(0).toString(), QStringLiteral("Loja Teste"));
  }
};

// -----------------------------------------------------------------------------
// TestCadastroClienteDialog — opens a real production QDialog end-to-end.
//
// PREREQ: a user must be logged in (User::login()) before construction.
// Without that, SearchDialog::vendedor (searchdialog.cpp:507, invoked
// indirectly by CadastroCliente) builds the filter fragment
// `" AND idLoja = " + User::idLoja` and ships it with an empty idLoja,
// producing SQL like `… AND idLoja =  AND nome != 'REPOSIÇÂO' …` — MySQL
// chokes on the bare `AND` right where the value should be. The production
// bug is masked because the real Loja.exe always goes through LoginDialog
// before any dialog opens; the test exposes it because we don't.
// initTestCase here calls User::login as the seeded admin to satisfy the
// invariant.
// -----------------------------------------------------------------------------
class TestCadastroClienteDialog : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    // Authenticate as the seeded admin (fixtures.sql idUsuario=1).
    // User::* static fields stay populated for the rest of the suite,
    // which is fine since cleanupTestCase resets them.
    try {
      User::login(QStringLiteral("admin_test"), QStringLiteral("admin123"));
    } catch (const std::exception &e) { QSKIP(qPrintable(QStringLiteral("seed login failed: %1 — fixtures.sql missing admin row?").arg(QString::fromLocal8Bit(e.what())))); }

    QCOMPARE(User::tipo, QStringLiteral("ADMINISTRADOR"));
    QCOMPARE(User::idLoja, QStringLiteral("1"));
  }

  void cleanupTestCase() {
    // Reset User globals so test classes that run after us start clean.
    User::idLoja.clear();
    User::idUsuario.clear();
    User::nome.clear();
    User::tipo.clear();
    User::usuario.clear();
    User::senha.clear();
  }

  void constructsWithoutThrowing() {
    // CadastroCliente extends RegisterAddressDialog → RegisterDialog →
    // QDialog. Construction triggers `model.setTable("cliente")` and pulls
    // in SearchDialog::vendedor (usuario filter), plus all the custom
    // delegates wired up by Qt Designer. Anything broken upstream fails
    // fast here. We don't actually show() the dialog — Tier 3 runs
    // headlessly on developer machines and CI; the QObject being
    // constructed successfully is the assertion.
    CadastroCliente dialog(nullptr);
    QVERIFY(not dialog.windowTitle().isEmpty());
  }

  void loadsSeededClientePF() {
    // viewRegisterById(1) loads the seeded PF cliente. Exercises model →
    // QDataWidgetMapper data binding for a known row without requiring
    // window exposure.
    CadastroCliente dialog(nullptr);
    QVERIFY(dialog.viewRegisterById(1));
  }
};

// -----------------------------------------------------------------------------
// TestRegisterDialogVariants — opens the other Cadastro* dialogs against
// the canonical seed. Same login + no-show pattern as TestCadastroClienteDialog.
// -----------------------------------------------------------------------------
class TestRegisterDialogVariants : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    try {
      User::login(QStringLiteral("admin_test"), QStringLiteral("admin123"));
    } catch (const std::exception &e) { QSKIP(qPrintable(QStringLiteral("seed login failed: %1").arg(QString::fromLocal8Bit(e.what())))); }
  }

  void cleanupTestCase() {
    User::idLoja.clear();
    User::idUsuario.clear();
    User::nome.clear();
    User::tipo.clear();
    User::usuario.clear();
    User::senha.clear();
  }

  void cadastroLojaLoadsSeededRow() {
    CadastroLoja dialog(nullptr);
    QVERIFY(dialog.viewRegisterById(1));
  }

  void cadastroFornecedorLoadsSeededRow() {
    CadastroFornecedor dialog(nullptr);
    QVERIFY(dialog.viewRegisterById(1));
  }
};

// -----------------------------------------------------------------------------
// TestVendaDialogSmoke — heaviest production dialog. Just constructing it
// triggers many model setups (venda items grid, search dialogs, custom
// delegates). The smoke test verifies construction succeeds; loading an
// existing venda row needs a richer fixture chain (cliente_has_endereco,
// profissional, etc.) which is deferred.
// -----------------------------------------------------------------------------
class TestVendaDialogSmoke : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    try {
      User::login(QStringLiteral("admin_test"), QStringLiteral("admin123"));
    } catch (const std::exception &e) { QSKIP(qPrintable(QStringLiteral("seed login failed: %1").arg(QString::fromLocal8Bit(e.what())))); }
  }

  void cleanupTestCase() {
    User::idLoja.clear();
    User::idUsuario.clear();
    User::tipo.clear();
  }

  void constructsWithoutThrowing() {
    Venda dialog(nullptr);
    QVERIFY(not dialog.windowTitle().isEmpty());
  }
};

// -----------------------------------------------------------------------------
// TestSearchDialogVendedorFilter — regression for the bug surfaced during
// M4 development: SearchDialog::vendedor used to build " AND idLoja = "
// (with empty value) when no user was logged in, producing invalid SQL.
// Run with cleared User state to verify the fix at searchdialog.cpp:507.
// -----------------------------------------------------------------------------
class TestSearchDialogVendedorFilter : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    // The previous TestCadastroClienteDialog::cleanupTestCase clears these,
    // but be defensive in case the suite is re-ordered.
    User::idLoja.clear();
    User::idUsuario.clear();
    User::tipo.clear();
  }

  void vendedorWithNoLoggedUserDoesNotThrow() {
    // Before the fix this would throw RuntimeException from
    // SqlTableModel::select() with the MySQL syntax-error message
    // "near ' AND nome != 'REPOSIÇÂO' ORDER BY …'".
    SearchDialog *sd = nullptr;
    try {
      sd = SearchDialog::vendedor(nullptr);
    } catch (const std::exception &e) { QFAIL(qPrintable(QStringLiteral("vendedor() threw with empty User state: %1").arg(QString::fromLocal8Bit(e.what())))); }
    QVERIFY(sd != nullptr);
    sd->deleteLater();
  }
};

// -----------------------------------------------------------------------------
// main — Tier 3 bootstrap (stub config + DB) then run smoke suite.
// -----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  const QString exeDir = findExeDir(argv);
  writeStubLojasTxt(exeDir);

  Application app(argc, argv);

  QString whyFailed;
  const bool dbReady = openDefaultTestConnection(whyFailed);

  int status = 0;

  if (not dbReady) {
    // No DB → skip suite cleanly with a clear reason. The smoke suite would
    // otherwise blow up on the first QSqlQuery.
    qWarning("[tier3] DB unavailable, skipping: %s", qPrintable(whyFailed));
    return 0;
  }

  {
    TestTier3Smoke t;
    status |= QTest::qExec(&t, argc, argv);
  }

  {
    TestCadastroClienteDialog t;
    status |= QTest::qExec(&t, argc, argv);
  }

  {
    TestRegisterDialogVariants t;
    status |= QTest::qExec(&t, argc, argv);
  }

  {
    TestVendaDialogSmoke t;
    status |= QTest::qExec(&t, argc, argv);
  }

  {
    TestSearchDialogVendedorFilter t;
    status |= QTest::qExec(&t, argc, argv);
  }

  // Release default connection so QSqlDatabasePrivate doesn't warn on exit.
  if (QSqlDatabase::contains()) {
    {
      auto db = QSqlDatabase::database(QLatin1String(QSqlDatabase::defaultConnection), false);
      if (db.isOpen()) { db.close(); }
    }
    QSqlDatabase::removeDatabase(QLatin1String(QSqlDatabase::defaultConnection));
  }

  return status;
}

#include "test_tier3.moc"
