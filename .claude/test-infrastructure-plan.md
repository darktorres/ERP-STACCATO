# Test Infrastructure for ERP Staccato

## Context

The codebase claims (in `CLAUDE.md`) to have "97 test methods covering critical functionality" in a `tests/` directory using Qt Test. **This is inaccurate.** No `tests/` directory exists; a prior attempt was reverted in commit `132ee103` and contained a single commented-out test method. There is currently **zero functional test coverage** across ~32.5 KLOC of Qt 5.15 C++ code (141 .cpp / 140 .h files) spanning Venda, Orçamento, Compras, Estoque, Financeiro, NFe, Logística, Galpão and Cadastros modules.

The intended outcome is a **pragmatic, three-tier test infrastructure** that can grow incrementally without forcing a sweeping refactor. The first deliverable is a green test binary running locally via `nmake check`; subsequent milestones add unit, integration, and UI smoke coverage targeting the highest-risk code (totals/freight in `venda.cpp` and `orcamento.cpp`, CPF/CNPJ validators, SQL string builders, stored-procedure-driven status transitions).

User decisions taken:
- **Scope:** full pyramid — unit (no DB) + integration (real MySQL) + UI smoke.
- **Build wiring:** Static lib + SUBDIRS (extract `src/` into `libstaccato`; `Loja.pro` and `tests/*.pro` both link against it).
- **CI:** Local-run only for now (no GitHub Actions workflow). Document `nmake check`.
- **DB fixture:** New `staccato_test` schema on the developer's local MySQL (no Docker).

## Test framework

**Qt Test** (`QTEST_MAIN`, `QCOMPARE`, `QVERIFY`, `QSignalSpy`, `QTest::keyClicks`, `QTest::mouseClick`). Already a transitive dependency via `QT += testlib`; required anyway for Tier 3 widget tests; no vendoring; `CONFIG += testcase` auto-wires `make check`.

## Build integration: `libstaccato` + SUBDIRS

Top-level orchestrator owns the build graph. `Loja.pro` shrinks to a thin app shell linking against `libstaccato`. Tests link the same lib — sources compile once, link twice. PCH stays owned by `libstaccato`.

### Files to create / rewrite

```
staccato.pro                      NEW   ~5 lines, SUBDIRS = libstaccato Loja tests
libstaccato.pro                   NEW   ~30 lines, TEMPLATE=lib CONFIG=staticlib
Loja.pro                          REWRITE  ~40 lines, TEMPLATE=app, just main.cpp + LIBS
tests/tests.pro                   NEW   TEMPLATE=subdirs, SUBDIRS = tier1 tier2 tier3
tests/common/integrationfixture.h NEW   QSqlDatabase test connection helper
tests/tier1/tier1.pro             NEW
tests/tier1/main.cpp              NEW
tests/tier1/test_validators.cpp   NEW
tests/tier2/tier2.pro             NEW   (added in M3)
tests/tier3/tier3.pro             NEW   (added in M4)
tests/fixtures/fixtures.sql       NEW   curated seed data (~200 INSERTs)
tests/README.md                   NEW   how to run locally
```

### `libstaccato.pro` shape

```pro
TEMPLATE = lib
CONFIG  += staticlib c++latest warn_on precompile_header
TARGET   = staccato
QT      *= core gui sql network xml charts widgets
PRECOMPILED_HEADER = pch.h
INCLUDEPATH += $$PWD/src
include(3rdparty/QtXlsxWriter/src/xlsx/qtxlsx.pri)
include(3rdparty/QSimpleUpdater/qsimpleupdater.pri)
include(3rdparty/LimeReport-1.5.68/limereport/limereport.pri)
SOURCES  = $$files(src/*.cpp, false)
SOURCES -= src/main.cpp
HEADERS  = $$files(src/*.h,   false)
FORMS    = $$files(ui/*.ui,   false)
RESOURCES = qrs/resources.qrc
# (re-include the win32-msvc / OpenSSL / cURL blocks currently in Loja.pro)
```

### `Loja.pro` after the shrink

`TEMPLATE = app`, `SOURCES = src/main.cpp`, `LIBS += -L../libstaccato -lstaccato`, plus Staccato.ico and version metadata. **Zero changes inside `src/`.**

## Three tiers

### Tier 1 — Pure unit (no DB, no event loop beyond `QGuiApplication`)

Target list, in implementation order. Existing locations referenced:

1. `validators::cpfValido()`, `cnpjValido()` — currently `RegisterDialog::validaCPF/validaCNPJ` at `src/registerdialog.cpp`. **Tiny refactor (M2):** add `src/validators.h/.cpp` with free static functions returning `bool`; dialog methods delegate. Avoids subclassing `RegisterDialog` just to probe.
2. `Application::roundDouble(double)` and overload — `src/application.cpp`. Pure, instance method only by convention.
3. `Application::ajustarDiaUtil(QDate)` — business-day adjustment, holidays/weekends.
4. `Application::sanitizeSQL(QString)` — escaping rules; covers SQL-injection regressions.
5. `Application::removerDiacriticos(QString, bool)` — Unicode normalization for search.
6. `Application::findTag(texto, tag)` — XML-ish tag extractor used by NFe parsing.
7. `LineEditTel::processTel()` + CEP formatter sibling — string → masked string.
8. `DateFormatDelegate::displayText()` — date formatting.
9. `SQL::contasPagar()`, `SQL::contasReceber()` query-string builders — snapshot-style assertions on generated `WHERE` clauses (`src/sql.cpp`).
10. `SQL::queryEstoque()`, `SQL::view_estoque_contabil()` — same pattern.
11. `Orcamento::calcularPeso()` — extract to a free function `orcamento_calc::calcularPeso(items)` (~20 LOC delta in M2), then test.
12. `Venda` total/rounding decomposition — extract the arithmetic out of `Venda::calcularTotais` into a pure helper taking a slice of line-item POD; test invariants (subtotal ≥ liquid, liquid ≤ total, rounding ≤ 0.01 R$ skew).

Tier 1 never opens `QSqlDatabase`. Any method that indirectly touches `qApp->db` belongs in Tier 2.

### Tier 2 — Integration against local MySQL (`staccato_test` schema)

**Connection seam — minimal refactor in `src/application.cpp`:**
- New `Application::dbConnectFromEnv()` (~15 LOC): if `STACCATO_TEST_DB_NAME` is set, skip the `mysql.txt` read and the `staccato`/`staccato_staging` switch; use env-supplied creds; connect to the named DB. Reuses the existing `db` member.
- Demote the "QMYSQL driver not available" hard exit at `application.cpp:43-49` to a thrown `RuntimeException` so the harness can surface it cleanly.

Env vars: `STACCATO_TEST_DB_HOST` (default `127.0.0.1`), `_PORT` (3306), `_USER`, `_PASS`, `_NAME` (default `staccato_test`).

**Schema bootstrap (one-time, manual):**
```sql
CREATE DATABASE staccato_test CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;
-- then from a shell:
-- mysql -uroot -p staccato_test < initdb.sql
-- mysql -uroot -p staccato_test < tests/fixtures/fixtures.sql
```

A short helper script `tests/scripts/reset-db.bat` wraps both loads for repeatability after destructive runs.

**`tests/fixtures/fixtures.sql`** — hand-curated canonical dataset (~200 INSERTs):
- 2 lojas, 3 usuários (admin/vendedor/financeiro), 5 produtos (one of each major class), 2 clientes (one PF with valid CPF, one PJ with valid CNPJ), 1 transportadora, 1 fornecedor. Committed to git.

**State isolation per test — transaction-rollback wrapper** in `tests/common/integrationfixture.h`:
```cpp
class IntegrationFixture : public QObject {
protected:
    QSqlDatabase db;
    void initTestCase()    { db = openTestConnection(); }
    void cleanupTestCase() { db.close(); }
    void init()            { db.transaction(); }
    void cleanup()         { db.rollback();   }
};
```

Sub-millisecond reset, full isolation. **Caveats specific to this schema:**
- `initdb.sql` defines 106 stored procedures / triggers / views (e.g. `update_venda_status`). Procedures that open their own transactions commit autonomously and break the rollback wrapper. Tests that `CALL` such procedures fall back to `truncateAffectedTables({...})` with `SET FOREIGN_KEY_CHECKS=0` and a reload from a serialized snapshot; tag those tests `[procedure]` and keep the count small.
- DDL is auto-commit — no test issues DDL, only DML.
- The schema is MySQL-only (InnoDB, ENUM, FULLTEXT, utf8mb4_0900_ai_ci, `DATE_ADD`/`INTERVAL`). SQLite is **not viable**; staying on local MySQL is the right call.

**Initial Tier 2 targets:**
- `cliente` insert + `validaCPF`/`validaCNPJ` round-trip.
- `Sql::contasPagar()` / `contasReceber()` builders executed against the real schema (proves the SQL parses).
- A Venda insert + `CALL update_venda_status()` side-effect assertion (the `[procedure]` tag pattern).
- Transaction rollback in `Application::rollbackTransaction()` actually reverts the row.

### Tier 3 — UI smoke

`QTest::qWaitForWindowExposed` → `keyClicks` / `mouseClick` against actual dialogs, connected to `staccato_test`. One happy-path per major dialog, no exhaustive coverage:
- Open `Venda` for a fixture `idVenda`; assert `ui->doubleSpinBoxTotal->value()` matches expected (proves end-to-end load + calculation).
- `Orcamento` freight: pick a transportadora, change peso, assert `doubleSpinBoxFrete` updates per `calcularFrete` rules.
- `RegisterDialog`-subclass (e.g. `CadastroCliente`) round-trip: type CPF → save → re-open → fields populated.

Lowest priority; added in M4 once Tier 2 is stable.

## Phased rollout

**M1 — Build pipeline proof (1 PR, ~2 days).** Add `staccato.pro`, `libstaccato.pro`, shrink `Loja.pro`. Add `tests/tier1/tier1.pro` + `test_validators.cpp` with one passing assertion using a `Probe` subclass of `RegisterDialog` (zero touches inside `src/`). Acceptance: `nmake check` from `tests/tier1/` prints `PASS : TestValidators::cpfValido()`. Production binary `Loja.exe` builds identically.

**M2 — Tier 1 coverage (2 PRs, ~1 week).** Extract `src/validators.h/.cpp` with free `cpfValido()/cnpjValido()`; rewire `RegisterDialog` to delegate. Add 8–10 tests across `Application` helpers and `SQL::*` builders. Extract `Orcamento::calcularPeso` and `Venda` arithmetic helpers; test.

**M3 — Tier 2 enablement (2 PRs, ~1 week).** Add `Application::dbConnectFromEnv()`. Demote QMYSQL hard-fail to throw. Add `tests/fixtures/fixtures.sql`, `tests/common/integrationfixture.h`, `tests/tier2/tier2.pro`. 3–4 tests including one `[procedure]` tagged.

**M4 — Tier 3 smoke + Venda/Orcamento invariants (ongoing).** One dialog test per major surface, added alongside future bug fixes (regression-driven coverage).

## First PR — concrete file list

Zero changes inside `src/`. Build glue + one trivial test that proves the link works end-to-end.

| File | Status | Approx. lines |
|---|---|---|
| `staccato.pro` | new | 5 |
| `libstaccato.pro` | new | 30 |
| `Loja.pro` | rewrite | 40 |
| `tests/tests.pro` | new | 3 (SUBDIRS) |
| `tests/tier1/tier1.pro` | new | 15 |
| `tests/tier1/main.cpp` | new | (in test_validators.cpp via QTEST_MAIN) |
| `tests/tier1/test_validators.cpp` | new | ~50 |
| `tests/README.md` | new | ~40 (how to build/run) |

`test_validators.cpp` sketch — uses a test-only subclass to access protected `validaCPF`/`validaCNPJ`. Acknowledged as a temporary expedient; M2 replaces with the free-function form.

```cpp
#include "registerdialog.h"
#include "application.h"
#include <QtTest>

class Probe : public RegisterDialog {
public:
    Probe() : RegisterDialog("cliente","idCliente",nullptr) {}
    using RegisterDialog::validaCPF;
    using RegisterDialog::validaCNPJ;
};

class TestValidators : public QObject {
    Q_OBJECT
private slots:
    void cpfValido()    { Probe p; QVERIFY(p.validaCPF("390.533.447-05")); }
    void cpfInvalido()  { Probe p;
        try { p.validaCPF("111.111.111-11"); QFAIL("should throw"); }
        catch (const RuntimeError &) {} }
    void cnpjValido()   { Probe p; QVERIFY(p.validaCNPJ("11.222.333/0001-81")); }
};
QTEST_MAIN(TestValidators)
#include "test_validators.moc"
```

## Critical files

- `Loja.pro` — rewritten to thin app shell
- `src/application.cpp` / `.h` — `dbConnectFromEnv` + demoted hard-fail (M3)
- `src/registerdialog.cpp` / `.h` — delegate validators (M2)
- `src/orcamento.cpp` — extract `calcularPeso` (M2)
- `src/venda.cpp` — extract total helpers (M2)
- `src/sql.cpp` — testable as-is (string builders)
- `initdb.sql` — loaded into `staccato_test` once

## Verification

**After M1 (build pipeline proof):**
1. From repo root: `qmake staccato.pro && nmake` builds `Loja.exe` identical to today.
2. `cd tests\tier1 && nmake check` runs the test binary and prints `PASS : TestValidators::cpfValido()` (and the two other validator tests).
3. Smoke-run the existing app to confirm packaging is unchanged.

**After M2 (Tier 1 expanded):**
- All Tier 1 tests pass under `nmake check`.
- Existing manual smoke flow on `Venda` and `Orcamento` still works (calculator extractions are non-functional changes).

**After M3 (Tier 2 enabled):**
- `mysql -uroot -p staccato_test < initdb.sql` then `< tests/fixtures/fixtures.sql` loads cleanly.
- With `STACCATO_TEST_DB_NAME=staccato_test` set, `tests\tier2\release\tier2_tests.exe` connects, runs tests, all PASS.
- After running, `SELECT COUNT(*) FROM cliente` in `staccato_test` returns the fixture count (i.e. rollback worked).

**After M4 (Tier 3 smoke):**
- `tier3_tests.exe` opens the actual `Venda` dialog, programmatically drives it, asserts UI invariants, closes cleanly.
