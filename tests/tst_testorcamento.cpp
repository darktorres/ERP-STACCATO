#include <QCoreApplication>
#include <QLineEdit>
#include <QtTest>

#include <QtWidgets/QDialog>

// add necessary includes here

#include "../app/src/logindialog.h"

class TestOrcamento : public QObject {
  Q_OBJECT

public:
  TestOrcamento();
  ~TestOrcamento();

private slots:
  void initTestCase();
  void cleanupTestCase();
  void test_case1();
};

TestOrcamento::TestOrcamento() {}

TestOrcamento::~TestOrcamento() {}

void TestOrcamento::initTestCase() {}

void TestOrcamento::cleanupTestCase() {}

void TestOrcamento::test_case1() {
  //  LoginDialog dialog;

  //  if (dialog.exec() == QDialog::Rejected) exit(1);

  //  auto login = dialog.findChild<QLineEdit *>("lineEditUser");

  //  if (login) {
  //    qDebug() << "ok!";
  //    QTest::keyClicks(login, "admin");
  //  }

  //  auto pass = dialog.findChild<QLineEdit *>("lineEditPass");

  //  if (pass) {
  //    qDebug() << "ok2!";
  //    QTest::keyClicks(pass, "Staccato900");
  //  }

  // lineEditUser
  // lineEditPass

  QVERIFY(1 == 1);
}

QTEST_MAIN(TestOrcamento)

#include "tst_testorcamento.moc"
