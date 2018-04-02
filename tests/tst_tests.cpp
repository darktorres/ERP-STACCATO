#include <QtTest>

// add necessary includes here
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

#include <application.h>
#include <logindialog.h>
#include <orcamento.h>

class Tests : public QObject {
  Q_OBJECT

public:
  Tests();
  ~Tests();

private slots:
  void initTestCase();
  void cleanupTestCase();
  void test_case1();
  void test_case2();
};

Tests::Tests() {}

Tests::~Tests() {}

void Tests::initTestCase() {}

void Tests::cleanupTestCase() {}

void Tests::test_case1() {
  LoginDialog dialog;

  auto user = dialog.findChild<QLineEdit *>("lineEditUser");

  QVERIFY(user);

  if (user) user->setText("admin");

  //  if (user) { QTest::keyClicks(user, "admin"); }

  auto pass = dialog.findChild<QLineEdit *>("lineEditPass");

  QVERIFY(pass);

  if (pass) pass->setText("900");

  //  if (pass) { QTest::keyClicks(pass, "900"); }

  auto host = dialog.findChild<QLineEdit *>("lineEditHostname");

  QVERIFY(host);

  if (host) host->setText("localhost");

  auto login = dialog.findChild<QPushButton *>("pushButtonLogin");

  QVERIFY(login);

  if (login) { login->click(); }
}

void Tests::test_case2() {
  Orcamento orcamento;
  orcamento.show();

  auto id = orcamento.findChild<QLineEdit *>("lineEditOrcamento");

  QVERIFY(id);

  QVERIFY(id->text() == "Auto gerado");
}

int main(int argc, char *argv[]) {
  Application app(argc, argv);
  app.setAttribute(Qt::AA_Use96Dpi, true);
  Tests tc;
  return QTest::qExec(&tc, argc, argv);
}

#include "tst_tests.moc"
