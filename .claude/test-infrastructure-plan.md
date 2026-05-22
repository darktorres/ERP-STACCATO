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

**M1 — Build pipeline proof (1 PR, ~2 days). [DONE]** Add `staccato.pro`, `libstaccato.pro`, shrink `Loja.pro` to a thin app shell linking against `libstaccato`. Add `tests/tier1/tier1.pro` + `test_smoke.cpp` containing one assertion (`QCOMPARE(1+1, 2)`). Acceptance: `nmake check` produces a binary that prints `PASS : TestSmoke::buildPipelineWorks()`. Production `Loja.exe` builds and starts up identically.

The originally planned `Probe`-subclass-of-`RegisterDialog` approach was dropped after discovering it crashes at runtime with `STATUS_DLL_INIT_FAILED`: referencing `RegisterDialog` forces the linker to pull in `Application`, QSimpleUpdater, LimeReport, etc., and something in that transitive set fails to initialize without the resource setup that lives in `Loja.exe`. M2's free-function extraction (next milestone) sidesteps the cascade entirely.

**M2 — Tier 1 coverage. [DONE]**
- M2.1: extracted `src/validators.h/.cpp`, delegated from `RegisterDialog`. 11 validator tests + 1 smoke.
- M2.2: extracted `src/app_helpers.h/.cpp` (`app::roundDouble`, `sanitizeSQL`, `removerDiacriticos`, `ajustarDiaUtil`). 15 tests + 6 `Sql::*` builder tests. DB-safety guard at end of `main()` asserts no `QSqlDatabase` was opened.
- M2.3: extracted `src/venda_calc.h/.cpp` (`venda_calc::calcularTotais` with `LineItem` POD); `Venda::calcularTotais` now reads rows from `modelItem` into a vector and delegates. Added `app::findTag` (`std::optional<QString>`-returning) — `Application::findTag` wraps with throw. 7 venda_calc tests + 5 findTag tests.
- **Total Tier 1: 51 PASS, 0 FAIL, 0 connections opened.**

Found a pre-existing bug while extracting `findTag`: the original `texto.indexOf(needle, Qt::CaseInsensitive)` passed the enum as the `from` offset (Qt's signature is `indexOf(str, from, cs)`), giving a case-sensitive search from index 0. Fixed in commit (TBD) — `findTag` now does case-insensitive matching as the original developer intended. Production SEFAZ events happen to ship exact case, so this is a no-op for known callers but more robust to future variation.

Layout note: tier1 tests aggregate into a single `test_tier1.cpp` with one custom `main()` running each `QObject` test class via `QTest::qExec`. Adding a new suite is one class + one block in `main()` — no `.pro` change.

Not extracted (intentionally):
- `Orcamento::calcularPeso`: not pure — queries the DB for `kgcx`. Only `caixas * kgcx` is arithmetic, not worth a separate helper.

**M3 — Tier 2 enablement. [M3.1 + M3.2 done]**
- M3.1: `tests/common/integration_fixture.{h,cpp}` with env-driven connection, hard refusal of `staccato`/`staccato_staging`, auto-bootstrap (CREATE DATABASE + load initdb.sql via `mysql.exe` if `loja` table absent). `tests/tier2/tier2.pro` + `test_tier2.cpp`. Hard guidance toward `mysql_native_password` because the bundled `libmysql.dll` lacks `caching_sha2_password.dll` plugin. Three bugs fixed during bring-up: `staccato` → `<dbname>` rewriting of `initdb.sql` at load time (1170 hardcoded refs), `START TRANSACTION` via raw SQL (Qt's `db.transaction()` short-circuits because libmysql doesn't advertise CLIENT_TRANSACTIONS), and `db = QSqlDatabase()` before `removeDatabase` to silence "still in use" warnings.
- M3.2: `tests/fixtures/fixtures.sql` curated seed (1 loja, 1 admin user, 1 cliente PF with CPF `390.533.447-05`, 1 cliente PJ with CNPJ `11.222.333/0001-81`). Loaded after `initdb.sql` only when the schema was just created (idempotent). `TestFixtures` (6 tests) verifies the canonical rows are present and bridges back to tier1 (`validators::cpfValido` accepts the seed CPF). `TestClienteRoundTrip` (4 tests) demonstrates transaction-rollback isolation across consecutive test functions.
- M3.3: stored-procedure smoke tests. While adding them, discovered that **zero** procedures/triggers in `initdb.sql` contain explicit `START TRANSACTION` / `COMMIT` / `ROLLBACK` (verified by grep) — they all run inside the caller's transaction. So the original plan's truncate-snapshot pattern turned out to be unnecessary: `IntegrationFixture`'s rollback wrapper handles procedures the same as any other DML. Shipped 5 procedure smoke tests (`update_venda_status`, `update_fornecedores_orcamento`, `update_fornecedores_venda`, `MYDATE()`, `SHA1_PASSWORD()`) verifying signatures and no-op handling without needing deep fixture chains.

Decisions taken in M3.1:
- Did **not** add `Application::dbConnectFromEnv()` — would force changes to Application's constructor that risk regressions in production startup. Test connection lives entirely in `tests/common/`, decoupled from Application.
- Did **not** demote the QMYSQL hard-fail (same reason: production behavior unchanged). Tests use `QApplication` directly, never instantiating Application, so they never hit the driver check.
- Auto-bootstrap shells out to `mysql.exe` via `QProcess` because `initdb.sql` uses `DELIMITER` and multi-statement procedures that `QSqlQuery` can't execute directly. Probe order: env var `STACCATO_TEST_MYSQL_BIN` → `QStandardPaths::findExecutable("mysql")` → hardcoded `C:\Program Files\MySQL\MySQL Server 8.4/8.0/5.7\bin\mysql.exe`.

**M4 — Tier 3 smoke + Venda/Orcamento invariants (ongoing).** One dialog test per major surface, added alongside future bug fixes (regression-driven coverage).

## First PR — concrete file list

Zero changes inside `src/`. Build glue + one trivial test that proves the link works end-to-end.

Actual files shipped:

| File | Status | Lines |
|---|---|---|
| `staccato.pro` | new | 23 |
| `libstaccato/libstaccato.pro` | new | 326 (mostly the explicit SOURCES/HEADERS/FORMS lists migrated from Loja.pro) |
| `Loja.pro` | rewrite | 91 (was 519) |
| `tests/tests.pro` | new | 11 (SUBDIRS) |
| `tests/tier1/tier1.pro` | new | 60 |
| `tests/tier1/test_smoke.cpp` | new | ~28 |
| `tests/README.md` | new | how to build/run + scope notes |

Decisions made during implementation, recorded for future-us:

- **LimeReport `lrfactoryinitializer.cpp`** is gated on `CONFIG=staticlib` in its `.pri`, but transitively needs `designer.pri` sources that aren't included. Workaround in `libstaccato.pro`: drop `staticlib` from CONFIG around the `include()` call, re-add it after.
- **Repo root must be on INCLUDEPATH** for both `libstaccato.pro` and `tests/tier1/tier1.pro`. The `.ui` files reference custom widgets as `<header>src/itembox.h</header>`; UIC emits `#include "src/itembox.h"` verbatim, which only resolves when the search path contains the repo root. (Legacy `Loja.pro` got this implicitly because cl ran from the repo root, so `-I.` covered it.)
- **All RESOURCES live in `Loja.pro`**, not in `libstaccato.pro`. Static-lib `.qrc` on MSVC needs explicit `Q_INIT_RESOURCE()` calls in `main()` to auto-init; keeping resources in the app sidesteps that. The 3rdparty `.pri` files get re-included in `Loja.pro` purely to harvest their RESOURCES; their SOURCES/HEADERS/FORMS are immediately cleared (already compiled in libstaccato).
- **`tests/tier1/tier1.pro` mirrors libstaccato's full QT module list** (`core gui sql network xml charts widgets testlib printsupport svg uitools qml`). The linker pulls in object files transitively from the static lib, and those need every module libstaccato itself uses. Saves debugging unresolved-symbol storms.
- **Build is x86 (32-bit)**, matching the legacy `.qmake.stash`. cURL libs only ship as x86 in `3rdparty/`; the original `Loja.pro` had the same asymmetry (cURL only added under `contains(QT_ARCH, i386)`).
- **CLAUDE.md MSVC path is stale**: VS 2022 BuildTools at `Microsoft Visual Studio\2022\BuildTools` no longer exists — the live install is under `\18\BuildTools`. Update CLAUDE.md alongside M2.

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
