#include <QDate>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QTimer>

#include "application.h"
#include "qsimpleupdater.h"
#include "usersession.h"

Application::Application(int &argc, char **argv, int) : QApplication(argc, argv) {
  setOrganizationName("Staccato");
  setApplicationName("ERP");
  setWindowIcon(QIcon("Staccato.ico"));
  setApplicationVersion("0.6.11");
  setStyle("Fusion");

  readSettingsFile();

  storeSelection();

  if (const auto tema = UserSession::getSetting("User/tema"); tema and tema.value().toString() == "escuro") { darkTheme(); }
}

void Application::readSettingsFile() {
  QFile file("lojas.txt");

  if (not file.open(QFile::ReadOnly)) {
    QMessageBox::critical(nullptr, "Erro!", "Erro lendo configurações: " + file.errorString());
    return;
  }

  const QStringList lines = QString(file.readAll()).split("\r\n", QString::SkipEmptyParts);

  for (int i = 0; i < lines.size(); i += 2) { mapLojas.insert(lines.at(i), lines.at(i + 1)); }
}

bool Application::dbConnect() {
  if (not QSqlDatabase::drivers().contains("QMYSQL")) {
    QMessageBox::critical(nullptr, "Não foi possível carregar o banco de dados.", "Este aplicativo requer o driver QMYSQL.");
    exit(1);
  }

  QSqlDatabase db = QSqlDatabase::contains() ? QSqlDatabase::database() : QSqlDatabase::addDatabase("QMYSQL");

  if (db.isOpen()) { return true; }

  const auto hostname = UserSession::getSetting("Login/hostname");

  if (not hostname) {
    QMessageBox::critical(nullptr, "Erro!", "A chave 'hostname' não está configurada!");
    return false;
  }

  const auto lastuser = UserSession::getSetting("User/lastuser");

  if (not lastuser) { return false; }

  db.setHostName(hostname.value().toString());
  db.setUserName(lastuser.value().toString().toLower());
  db.setPassword("12345");
  db.setDatabaseName("mydb");
  db.setPort(3306);

  db.setConnectOptions("CLIENT_COMPRESS=1");

  QSqlQuery query;

  if (not db.open()) {
    // TODO: caso o servidor selecionado nao esteja disponivel tente os outros
    // TODO: try local ip's first

    //    db.setHostName("201.6.117.16");
    //    UserSession::setSetting("Login/hostname", "201.6.117.16");

    //    if (not db.open()) {
    //      db.setHostName("187.122.102.193");
    //      UserSession::setSetting("Login/hostname", "187.122.102.193");

    //      if (not db.open()) {
    //        QMessageBox::critical(nullptr, "Erro", "Erro conectando no banco de dados: " + db.lastError().text());
    //        isConnected = false;
    //        return false;
    //      }
    //    }

    //      //    QMessageBox::critical(nullptr, "Erro", "Erro conectando no banco de dados: " + db.lastError().text());
    //      //    return false;
    //    }

    QMessageBox::critical(nullptr, "Erro", "Erro conectando no banco de dados: " + db.lastError().text());

    isConnected = false;

    return false;
  }

  isConnected = true;

  if (not query.exec("SELECT lastInvalidated FROM maintenance") or not query.first()) {
    QMessageBox::critical(nullptr, "Erro", "Erro verificando lastInvalidated: " + query.lastError().text());
    return false;
  }

  if (query.value("lastInvalidated").toDate() < QDate::currentDate()) {
    if (not query.exec("CALL invalidar_produtos_expirados()")) {
      QMessageBox::critical(nullptr, "Erro!", "Erro executando InvalidarExpirados: " + query.lastError().text());
      return false;
    }

    if (not query.exec("CALL invalidar_orcamentos_expirados()")) {
      QMessageBox::critical(nullptr, "Erro!", "Erro executando update_orcamento_status: " + query.lastError().text());
      return false;
    }

    query.prepare("UPDATE maintenance SET lastInvalidated = :lastInvalidated WHERE id = 1");
    query.bindValue(":lastInvalidated", QDate::currentDate().toString("yyyy-MM-dd"));

    if (not query.exec()) {
      QMessageBox::critical(nullptr, "Erro", "Erro atualizando lastInvalidated: " + query.lastError().text());
      return false;
    }
  }

  startSqlPing();

  return true;
}

void Application::startSqlPing() {
  auto *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [] { QSqlQuery("DO 0").exec(); });
  timer->start(60000);

  // TODO: futuramente verificar se tem atualizacao no servidor e avisar usuario
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

  UserSession::setSetting("User/tema", "escuro");
}

void Application::lightTheme() {
  qApp->setStyle("Fusion");
  qApp->setPalette(defaultPalette);
  qApp->setStyleSheet(styleSheet());

  UserSession::setSetting("User/tema", "claro");
}

void Application::endTransaction() {
  inTransaction = false;

  showMessages();
}

void Application::enqueueWarning(const QString &warning) {
  warningQueue << warning;

  if (not updating) { showMessages(); }
}

void Application::enqueueInformation(const QString &information) {
  informationQueue << information;

  if (not updating) { showMessages(); }
}

void Application::enqueueError(const QString &error) {
  errorQueue << error;

  if (not updating) { showMessages(); }
}

// TODO: if inTransaction is true give a error message and abort (to avoid nested transactions)
void Application::startTransaction() { inTransaction = true; }

bool Application::getShowingErrors() const { return showingErrors; }

bool Application::getIsConnected() const { return isConnected; }

QMap<QString, QString> Application::getMapLojas() const { return mapLojas; }

void Application::storeSelection() {
  if (not UserSession::getSetting("Login/hostname")) {
    const QStringList items = mapLojas.keys();

    const QString loja = QInputDialog::getItem(nullptr, "Escolha a loja", "Qual a sua loja?", items, 0, false);

    if (loja.isEmpty()) { return; }

    UserSession::setSetting("Login/hostname", mapLojas.value(loja));
  }
}

bool Application::getInTransaction() const { return inTransaction; }

void Application::setInTransaction(const bool value) { inTransaction = value; }

void Application::setUpdating(const bool value) {
  updating = value;

  if (value == false) { showMessages(); }
}

bool Application::getUpdating() const { return updating; }

void Application::showMessages() {
  if (inTransaction or updating) { return; }
  if (errorQueue.isEmpty() and warningQueue.isEmpty() and informationQueue.isEmpty()) { return; }

  showingErrors = true;

  // TODO: deal with 'Lost connection to MySQL server'

  for (const auto &error : std::as_const(errorQueue)) { QMessageBox::critical(nullptr, "Erro!", error); }
  for (const auto &warning : std::as_const(warningQueue)) { QMessageBox::warning(nullptr, "Aviso!", warning); }
  for (const auto &information : std::as_const(informationQueue)) { QMessageBox::information(nullptr, "Informação!", information); }

  errorQueue.clear();
  warningQueue.clear();
  informationQueue.clear();

  showingErrors = false;
}

void Application::updater() {
  const auto hostname = UserSession::getSetting("Login/hostname");

  if (not hostname) { return; }

  auto *updater = new QSimpleUpdater();
  updater->setApplicationVersion(qApp->applicationVersion());
  updater->setReferenceUrl("http://" + hostname.value().toString() + "/versao.txt");
  updater->setDownloadUrl("http://" + hostname.value().toString() + "/Instalador.exe");
  updater->setSilent(true);
  updater->setShowNewestVersionMessage(true);
  updater->checkForUpdates();
}
