// Tier 1 — pure unit tests aggregated into a single binary. Add more test
// classes here as M2 progresses (Application helpers, SQL builders, etc.).
//
// Pattern: each test class is a QObject with private slots; a single main()
// runs them all and ORs the exit statuses. We use QApplication (not plain
// QCoreApplication) because the legacy code occasionally constructs widgets
// in static init paths, and QApplication is cheap enough on Tier 1.

#include "validators.h"

#include <QApplication>
#include <QtTest>

// -----------------------------------------------------------------------------
// TestSmoke — proves the build/link/run pipeline. Never depends on libstaccato
// internals.
// -----------------------------------------------------------------------------
class TestSmoke : public QObject {
  Q_OBJECT

private slots:
  void buildPipelineWorks() { QCOMPARE(1 + 1, 2); }
};

// -----------------------------------------------------------------------------
// TestValidators — exercises validators::cpfValido / cnpjValido via the free
// functions (M2 extraction). The free form returns bool uniformly, so we can
// assert both valid and invalid inputs without crashing on throw paths.
// -----------------------------------------------------------------------------
class TestValidators : public QObject {
  Q_OBJECT

private slots:
  // ---- CPF -----------------------------------------------------------------

  void cpfValidoFormatado() { QVERIFY(validators::cpfValido(QStringLiteral("390.533.447-05"))); }

  void cpfValidoSemFormatacao() { QVERIFY(validators::cpfValido(QStringLiteral("39053344705"))); }

  void cpfTamanhoErrado() {
    QVERIFY(not validators::cpfValido(QString()));
    QVERIFY(not validators::cpfValido(QStringLiteral("123")));
    QVERIFY(not validators::cpfValido(QStringLiteral("123456789012")));
  }

  void cpfTodosDigitosIguais() {
    // Sequences like 111.111.111-11 satisfy the checksum algorithm but are
    // explicitly rejected by Receita Federal — keep that special case.
    QVERIFY(not validators::cpfValido(QStringLiteral("00000000000")));
    QVERIFY(not validators::cpfValido(QStringLiteral("111.111.111-11")));
    QVERIFY(not validators::cpfValido(QStringLiteral("99999999999")));
  }

  void cpfChecksumInvalido() {
    // Same as the valid one but last digit flipped.
    QVERIFY(not validators::cpfValido(QStringLiteral("390.533.447-04")));
    // And first check digit flipped.
    QVERIFY(not validators::cpfValido(QStringLiteral("390.533.447-15")));
  }

  // ---- CNPJ ----------------------------------------------------------------

  void cnpjValidoFormatado() { QVERIFY(validators::cnpjValido(QStringLiteral("11.222.333/0001-81"))); }

  void cnpjValidoSemFormatacao() { QVERIFY(validators::cnpjValido(QStringLiteral("11222333000181"))); }

  void cnpjTamanhoErrado() {
    QVERIFY(not validators::cnpjValido(QString()));
    QVERIFY(not validators::cnpjValido(QStringLiteral("123")));
    QVERIFY(not validators::cnpjValido(QStringLiteral("112223330001811")));
  }

  void cnpjChecksumInvalido() {
    // Valid base, last digit flipped.
    QVERIFY(not validators::cnpjValido(QStringLiteral("11.222.333/0001-82")));
    // Valid base, first check digit flipped.
    QVERIFY(not validators::cnpjValido(QStringLiteral("11.222.333/0001-91")));
  }
};

// -----------------------------------------------------------------------------
// Aggregating main — keeps a single .exe so adding test classes doesn't need
// build-system changes.
// -----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setAttribute(Qt::AA_Use96Dpi, true);

  int status = 0;

  {
    TestSmoke t;
    status |= QTest::qExec(&t, argc, argv);
  }

  {
    TestValidators t;
    status |= QTest::qExec(&t, argc, argv);
  }

  return status;
}

#include "test_tier1.moc"
