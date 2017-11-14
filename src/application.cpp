#include <QDate>
#include <QDebug>
#include <QIcon>
#include <QMessageBox>
#include <QSqlError>

#include "application.h"
#include "usersession.h"

Application::Application(int &argc, char **argv, int) : QApplication(argc, argv) {
  setOrganizationName("Staccato");
  setApplicationName("ERP");
  setWindowIcon(QIcon("Staccato.ico"));
  setApplicationVersion("0.5.46");
  setStyle("Fusion");

  dbConnect();

  if (UserSession::getSetting("User/tema").toString() == "escuro") darkTheme();
}

bool Application::dbConnect() {
  if (not QSqlDatabase::drivers().contains("QMYSQL")) {
    QMessageBox::critical(nullptr, "Não foi possível carregar o banco de dados.", "Este aplicativo requer o driver QMYSQL.");
    exit(1);
  }

  QSqlDatabase db = QSqlDatabase::contains() ? QSqlDatabase::database() : QSqlDatabase::addDatabase("QMYSQL");

  db.setHostName(UserSession::getSetting("Login/hostname").toString());
  db.setUserName(UserSession::getSetting("User/lastuser").toString().toLower());
  db.setPassword("1234");
  db.setDatabaseName("mysql");
  db.setPort(3306);

  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_RECONNECT=1");
  //  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_RECONNECT=1;MYSQL_OPT_CONNECT_TIMEOUT=60;MYSQL_OPT_READ_TIMEOUT=60;"
  //                       "MYSQL_OPT_WRITE_TIMEOUT=60");

  if (not db.open()) {
    QMessageBox::critical(nullptr, "Erro", "Erro conectando no banco de dados: " + db.lastError().text());
    return false;
  }

  QSqlQuery query = db.exec("SHOW SCHEMAS");
  bool hasMydb = false;

  while (query.next()) {
    if (query.value(0).toString() == "mydb") hasMydb = true;
  }

  if (not hasMydb) {
    QMessageBox::critical(nullptr, "Erro!", "Não encontrou as tabelas do bando de dados, verifique se o servidor está funcionando corretamente.");
    return false;
  }

  db.close();

  db.setDatabaseName("mydb");

  if (not db.open()) {
    QMessageBox::critical(nullptr, "Erro", "Erro conectando no banco de dados: " + db.lastError().text());
    return false;
  }

  if (not query.exec("SELECT lastInvalidated FROM maintenance") or not query.first()) {
    QMessageBox::critical(nullptr, "Erro", "Erro verificando lastInvalidated: " + query.lastError().text());
    return false;
  }

  if (query.value("lastInvalidated").toDate() < QDate::currentDate()) {
    if (not query.exec("CALL invalidate_expired()")) {
      QMessageBox::critical(nullptr, "Erro!", "Erro executando InvalidarExpirados: " + query.lastError().text());
      return false;
    }

    query.prepare("UPDATE maintenance SET lastInvalidated = :lastInvalidated WHERE id = 1");
    query.bindValue(":lastInvalidated", QDate::currentDate().toString("yyyy-MM-dd"));

    if (not query.exec()) {
      QMessageBox::critical(nullptr, "Erro", "Erro atualizando lastInvalidated: " + query.lastError().text());
      return false;
    }
  }

  if (not query.exec("CALL update_orcamento_status()")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro executando update_orcamento_status: " + query.lastError().text());
    return false;
  }

  return true;
}

void Application::darkTheme() {
  qApp->setStyle("Fusion");

  QPalette darkPalette;
  darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::WindowText, Qt::white);
  darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
  darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
  darkPalette.setColor(QPalette::ToolTipText, Qt::white);
  darkPalette.setColor(QPalette::Text, Qt::white);
  darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ButtonText, Qt::white);
  darkPalette.setColor(QPalette::BrightText, Qt::red);
  darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));

  darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::HighlightedText, Qt::black);

  darkPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(120, 120, 120));
  darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120));

  qApp->setPalette(darkPalette);

  qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
}

void Application::lightTheme() {
  qApp->setStyle("Fusion");
  qApp->setPalette(defaultPalette);
  qApp->setStyleSheet(styleSheet());
}

void Application::endTransaction() {
  inTransaction = false;

  showErrors();
}

void Application::enqueueError(const QString &error) {
  errorQueue << error;

  showErrors();
}

void Application::startTransaction() { inTransaction = true; }

bool Application::getInTransaction() const { return inTransaction; }

void Application::setInTransaction(const bool value) { inTransaction = value; }

void Application::setUpdating(const bool value) { updating = value; }

bool Application::getUpdating() const { return updating; }

void Application::showErrors() {
  if (inTransaction or updating) return;

  for (const auto &error : errorQueue) QMessageBox::critical(nullptr, "Erro!", error);

  errorQueue.clear();
}
