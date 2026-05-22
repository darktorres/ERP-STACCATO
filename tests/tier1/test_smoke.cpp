// M1 build-pipeline proof. The test deliberately exercises NO libstaccato code
// — its only job is to confirm:
//   1. qmake + nmake on staccato.pro produce a runnable Qt Test binary.
//   2. The SUBDIRS/lib layout links cleanly.
//   3. nmake check (CONFIG += testcase) wires up correctly.
//
// Real coverage starts in M2:
//   - Extract validaCPF/validaCNPJ to free functions in src/validators.h/.cpp
//     (so they're testable without instantiating RegisterDialog, which
//     transitively drags Application + QSimpleUpdater + LimeReport into
//     the test binary and triggers a DLL_INIT_FAILED at startup).
//   - Add tests for Application::roundDouble, ajustarDiaUtil, sanitizeSQL,
//     SQL::contasPagar / contasReceber builders, etc.
//
// See .claude/test-infrastructure-plan.md for the full milestone plan.

#include <QtTest>

class TestSmoke : public QObject {
  Q_OBJECT

private slots:
  void buildPipelineWorks() { QCOMPARE(1 + 1, 2); }
};

QTEST_MAIN(TestSmoke)
#include "test_smoke.moc"
