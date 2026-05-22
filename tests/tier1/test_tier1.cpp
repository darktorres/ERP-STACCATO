// Tier 1 — pure unit tests aggregated into a single binary. Add more test
// classes here as M2 progresses (Application helpers, SQL builders, etc.).
//
// Pattern: each test class is a QObject with private slots; a single main()
// runs them all and ORs the exit statuses. We use QApplication (not plain
// QCoreApplication) because the legacy code occasionally constructs widgets
// in static init paths, and QApplication is cheap enough on Tier 1.

#include "app_helpers.h"
#include "sql.h"
#include "validators.h"
#include "venda_calc.h"

#include <QApplication>
#include <QSqlDatabase>
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
// TestAppHelpers — pure helpers extracted from Application (M2.2).
// -----------------------------------------------------------------------------
class TestAppHelpers : public QObject {
  Q_OBJECT

private slots:
  // ---- roundDouble ---------------------------------------------------------

  void roundDoubleDefault() {
    // Default = 4 decimal places.
    QCOMPARE(app::roundDouble(1.23456789), 1.2346);
    QCOMPARE(app::roundDouble(1.23454999), 1.2345);
  }

  void roundDoubleCustomDecimals() {
    QCOMPARE(app::roundDouble(1.5, 0), 2.0);
    QCOMPARE(app::roundDouble(1.234, 2), 1.23);
    QCOMPARE(app::roundDouble(1.236, 2), 1.24);
  }

  void roundDoubleHalfAwayFromZero() {
    // std::round semantics: 0.5 → 1, -0.5 → -1 (away from zero).
    QCOMPARE(app::roundDouble(0.5, 0), 1.0);
    QCOMPARE(app::roundDouble(-0.5, 0), -1.0);
  }

  void roundDoubleZero() { QCOMPARE(app::roundDouble(0.0), 0.0); }

  // ---- sanitizeSQL ---------------------------------------------------------

  void sanitizeSQLStripsSpecialChars() {
    // Documents the historical strip set: + @ > < ~ * ' \ — production code
    // depends on these being removed before string concatenation.
    QCOMPARE(app::sanitizeSQL(QStringLiteral("a+b@c>d<e~f*g'h\\i")), QStringLiteral("abcdefghi"));
  }

  void sanitizeSQLKeepsSafeChars() {
    // Spaces, parens, digits, alphanumerics must pass through untouched.
    QCOMPARE(app::sanitizeSQL(QStringLiteral("Hello, world! (42)")), QStringLiteral("Hello, world! (42)"));
  }

  void sanitizeSQLEmpty() { QCOMPARE(app::sanitizeSQL(QString()), QString()); }

  // ---- removerDiacriticos --------------------------------------------------

  void removerDiacriticosBasic() {
    QCOMPARE(app::removerDiacriticos(QString::fromUtf8("ação")), QStringLiteral("acao"));
    QCOMPARE(app::removerDiacriticos(QString::fromUtf8("São Paulo")), QStringLiteral("Sao Paulo"));
  }

  void removerDiacriticosNoChange() {
    QCOMPARE(app::removerDiacriticos(QStringLiteral("plain ascii")), QStringLiteral("plain ascii"));
  }

  void removerDiacriticosWithSymbols() {
    // removerSimbolos=true also drops punctuation, keeps alphanumeric + space.
    QCOMPARE(app::removerDiacriticos(QString::fromUtf8("São, Paulo! 123"), true), QStringLiteral("Sao Paulo 123"));
  }

  void removerDiacriticosEmpty() {
    QCOMPARE(app::removerDiacriticos(QString()), QString());
    QCOMPARE(app::removerDiacriticos(QString(), true), QString());
  }

  // ---- ajustarDiaUtil ------------------------------------------------------

  void ajustarDiaUtilWeekdayUnchanged() {
    // 2026-05-20 is a Wednesday — weekday → unchanged.
    const QDate wed(2026, 5, 20);
    QCOMPARE(app::ajustarDiaUtil(wed), wed);
  }

  void ajustarDiaUtilSaturdayAdvancesToMonday() {
    // 2026-05-23 is a Saturday → expect 2026-05-25 (Mon).
    QCOMPARE(app::ajustarDiaUtil(QDate(2026, 5, 23)), QDate(2026, 5, 25));
  }

  void ajustarDiaUtilSundayAdvancesToMonday() {
    QCOMPARE(app::ajustarDiaUtil(QDate(2026, 5, 24)), QDate(2026, 5, 25));
  }

  void ajustarDiaUtilFridayUnchanged() {
    QCOMPARE(app::ajustarDiaUtil(QDate(2026, 5, 22)), QDate(2026, 5, 22));
  }

  // ---- findTag -------------------------------------------------------------

  void findTagSimple() {
    const QString texto = QStringLiteral("preamble\r\ncStat: 100\r\nxMotivo: OK");
    const auto found = app::findTag(texto, QStringLiteral("cStat:"));
    QVERIFY(found.has_value());
    QCOMPARE(*found, QStringLiteral(" 100"));
  }

  void findTagIsCaseInsensitive() {
    // Original code intended case-insensitive matching but had the
    // arguments wrong (Qt::CaseInsensitive landed in the `from` offset).
    // Bug fixed; verify both casings match.
    const QString texto = QStringLiteral("aaa\r\nCSTAT:100");
    QVERIFY(app::findTag(texto, QStringLiteral("cstat:")).has_value());
    QVERIFY(app::findTag(texto, QStringLiteral("CSTAT:")).has_value());
    QCOMPARE(*app::findTag(texto, QStringLiteral("cstat:")), QStringLiteral("100"));
  }

  void findTagMissing() { QVERIFY(not app::findTag(QStringLiteral("nope\r\nfoo"), QStringLiteral("bar:")).has_value()); }

  void findTagRequiresPrecedingCRLF() {
    // The tag must be CRLF-prefixed — a match at index 0 (no preceding CRLF)
    // is NOT found. Documents legacy behaviour; callers prepend "\r\n" if the
    // tag could be the first line.
    QVERIFY(not app::findTag(QStringLiteral("cStat: 100\r\nxMotivo: OK"), QStringLiteral("cStat:")).has_value());
  }

  void findTagStopsAtNextCRLF() {
    const QString texto = QStringLiteral("a\r\ntag:value\r\nnext:other");
    QCOMPARE(*app::findTag(texto, QStringLiteral("tag:")), QStringLiteral("value"));
  }
};

// -----------------------------------------------------------------------------
// TestVendaCalc — extracted pure arithmetic for Venda::calcularTotais (M2.3).
// -----------------------------------------------------------------------------
class TestVendaCalc : public QObject {
  Q_OBJECT

private slots:
  void empty() {
    const auto [bruto, liq, total] = venda_calc::calcularTotais({});
    QCOMPARE(bruto, 0.0);
    QCOMPARE(liq, 0.0);
    QCOMPARE(total, 0.0);
  }

  void singleRow() {
    const auto [bruto, liq, total] = venda_calc::calcularTotais({
        {100.0, 90.0, 95.0, false},
    });
    QCOMPARE(bruto, 100.0);
    QCOMPARE(liq, 90.0);
    QCOMPARE(total, 95.0);
  }

  void multipleRowsSummed() {
    const auto [bruto, liq, total] = venda_calc::calcularTotais({
        {100.0, 90.0, 95.0, false},
        {200.0, 180.0, 195.0, false},
        {50.0, 50.0, 50.0, false},
    });
    QCOMPARE(bruto, 350.0);
    QCOMPARE(liq, 320.0);
    QCOMPARE(total, 340.0);
  }

  void pendingDeletionSkipped() {
    // Row marked for deletion must not contribute to any column.
    const auto [bruto, liq, total] = venda_calc::calcularTotais({
        {100.0, 90.0, 95.0, false},
        {999.0, 999.0, 999.0, true}, // ← deletion-pending, must be ignored
        {200.0, 180.0, 195.0, false},
    });
    QCOMPARE(bruto, 300.0);
    QCOMPARE(liq, 270.0);
    QCOMPARE(total, 290.0);
  }

  void invariantSubTotalBrutoGeSubTotalLiq() {
    // For any non-deletion row, parcial (bruto) ≥ parcialDesc (líquido)
    // is expected business invariant. The helper itself is a plain sum —
    // this test documents the invariant at the data-shape level.
    const QVector<venda_calc::LineItem> items = {
        {100.0, 90.0, 95.0, false},
        {50.0, 50.0, 50.0, false},
        {25.0, 20.0, 22.0, false},
    };
    const auto [bruto, liq, total] = venda_calc::calcularTotais(items);
    QVERIFY(bruto >= liq);
    QVERIFY(total >= 0.0);
  }
};

// -----------------------------------------------------------------------------
// TestSqlBuilders — Sql:: query-string builders (M2.2).
//
// These functions take filter/search strings and produce SQL text. They never
// touch the DB; testing them is a snapshot assertion on substring shape.
// -----------------------------------------------------------------------------
class TestSqlBuilders : public QObject {
  Q_OBJECT

private slots:
  void contasPagarNoFilter() {
    // Empty filter → no WHERE clause.
    const QString sql = Sql::contasPagar(QString(), QString());
    QVERIFY(sql.contains(QStringLiteral("FROM")));
    QVERIFY(sql.contains(QStringLiteral("conta_a_pagar_has_pagamento")));
    QVERIFY(not sql.contains(QStringLiteral("WHERE")));
  }

  void contasPagarWithFilter() {
    const QString sql = Sql::contasPagar(QStringLiteral("cp.status = 'PENDENTE'"), QString());
    QVERIFY(sql.contains(QStringLiteral("WHERE cp.status = 'PENDENTE'")));
  }

  void contasPagarWithSearch() {
    // `busca` is appended after the outer subquery, used for HAVING/extra
    // ordering by the caller. Test that it lands at the tail.
    const QString sql = Sql::contasPagar(QString(), QStringLiteral("HAVING ordemCompra LIKE '%x%'"));
    QVERIFY(sql.endsWith(QStringLiteral("HAVING ordemCompra LIKE '%x%'")));
  }

  void contasReceberNoFilter() {
    const QString sql = Sql::contasReceber(QString());
    QVERIFY(sql.contains(QStringLiteral("conta_a_receber_has_pagamento")));
    QVERIFY(not sql.contains(QStringLiteral("WHERE")));
    QVERIFY(sql.contains(QStringLiteral("GROUP BY")));
  }

  void contasReceberWithFilter() {
    const QString sql = Sql::contasReceber(QStringLiteral("cr.status = 'RECEBIDO'"));
    QVERIFY(sql.contains(QStringLiteral("WHERE cr.status = 'RECEBIDO'")));
  }

  void queryEstoqueWithMatch() {
    // queryEstoque takes `match` (free-form text) and `having` (post-aggr).
    const QString sql = Sql::queryEstoque(QStringLiteral("AND e.idLoja = 1"), QString());
    QVERIFY(sql.contains(QStringLiteral("AND e.idLoja = 1")));
    QVERIFY(sql.contains(QStringLiteral("FROM")));
  }
};

// -----------------------------------------------------------------------------
// Aggregating main — keeps a single .exe so adding test classes doesn't need
// build-system changes.
//
// SAFETY: Tier 1 must NEVER open a QSqlDatabase. The production schemas
// `staccato` and `staccato_staging` hold real data, and initdb.sql defines
// 106 stored procedures/triggers — an accidental connection could fire side
// effects. We assert at end of run that no connection was ever registered.
// Tier 2 (M3) will introduce DB access through a separate `staccato_test`
// schema and a dedicated `IntegrationFixture` base class, which must never
// be linked into Tier 1.
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

  {
    TestAppHelpers t;
    status |= QTest::qExec(&t, argc, argv);
  }

  {
    TestSqlBuilders t;
    status |= QTest::qExec(&t, argc, argv);
  }

  {
    TestVendaCalc t;
    status |= QTest::qExec(&t, argc, argv);
  }

  if (not QSqlDatabase::connectionNames().isEmpty()) {
    qCritical("Tier 1 must not open a QSqlDatabase. Connections found: %s",
              qPrintable(QSqlDatabase::connectionNames().join(QStringLiteral(", "))));
    status |= 1;
  }

  return status;
}

#include "test_tier1.moc"
