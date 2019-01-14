#include <QDate>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QStyle>
#include <QTimer>

#include "application.h"
#include "qsimpleupdater.h"
#include "usersession.h"

Application::Application(int &argc, char **argv, int) : QApplication(argc, argv) {
  setOrganizationName("Staccato");
  setApplicationName("ERP");
  setWindowIcon(QIcon("Staccato.ico"));
  setApplicationVersion("0.6.40");
  setStyle("Fusion");

  readSettingsFile();

  storeSelection();

  if (const auto tema = UserSession::getSetting("User/tema"); tema and tema.value().toString() == "escuro") { darkTheme(); }
}

void Application::enqueueError(const QString &error, QWidget *parent) {
  errorQueue << Message{error, parent};

  if (not updating) { showMessages(); }
}

bool Application::enqueueError(const bool boolean, const QString &error, QWidget *parent) {
  enqueueError(error, parent);
  return boolean;
}

void Application::enqueueInformation(const QString &information, QWidget *parent) {
  informationQueue << Message{information, parent};

  if (not updating) { showMessages(); }
}

void Application::enqueueWarning(const QString &warning, QWidget *parent) {
  warningQueue << Message{warning, parent};

  if (not updating) { showMessages(); }
}

void Application::readSettingsFile() {
  QFile file("lojas.txt");

  if (not file.open(QFile::ReadOnly)) { return qApp->enqueueError("Erro lendo configurações: " + file.errorString()); }

  const QStringList lines = QString(file.readAll()).split("\r\n", QString::SkipEmptyParts);

  for (int i = 0; i < lines.size(); i += 2) { mapLojas.insert(lines.at(i), lines.at(i + 1)); }
}

bool Application::setDatabase() {
  if (not QSqlDatabase::drivers().contains("QMYSQL")) {
    qApp->enqueueError("Este aplicativo requer o driver QMYSQL.");
    exit(1);
  }

  const auto hostname = UserSession::getSetting("Login/hostname");

  if (not hostname) { return qApp->enqueueError(false, "A chave 'hostname' não está configurada!"); }

  const auto lastuser = UserSession::getSetting("User/lastuser");

  if (not lastuser) { return false; }

  db = QSqlDatabase::contains() ? QSqlDatabase::database() : QSqlDatabase::addDatabase("QMYSQL");

  db.setHostName(hostname.value().toString());
  db.setUserName(lastuser.value().toString().toLower());
  db.setPassword("123456");
  db.setDatabaseName("mydb");
  db.setPort(3306);

  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_READ_TIMEOUT=10;MYSQL_OPT_WRITE_TIMEOUT=10;MYSQL_OPT_CONNECT_TIMEOUT=10");

  return true;
}

bool Application::dbReconnect() {
  db.close();

  if (not db.open()) {
    isConnected = false;
    return qApp->enqueueError(false, "Erro conectando no banco de dados: " + db.lastError().text());
  }

  return db.isOpen();
}

bool Application::dbConnect() {
  if (not setDatabase()) { return false; }

  if (not db.open()) {
    // TODO: caso o servidor selecionado nao esteja disponivel tente os outros
    // TODO: try local ip's first

    isConnected = false;

    return qApp->enqueueError(false, "Erro conectando no banco de dados: " + db.lastError().text());
  }

  isConnected = true;

  if (not runSqlJobs()) { return false; }

  startSqlPing();
  // startUpdaterPing();

  return true;
}

bool Application::runSqlJobs() {
  QSqlQuery query;

  if (not query.exec("SELECT lastInvalidated FROM maintenance") or not query.first()) { return qApp->enqueueError(false, "Erro verificando lastInvalidated: " + query.lastError().text()); }

  if (query.value("lastInvalidated").toDate() < QDate::currentDate()) {
    if (not query.exec("CALL invalidar_produtos_expirados()")) { return qApp->enqueueError(false, "Erro executando InvalidarExpirados: " + query.lastError().text()); }

    if (not query.exec("CALL invalidar_orcamentos_expirados()")) { return qApp->enqueueError(false, "Erro executando update_orcamento_status: " + query.lastError().text()); }

    query.prepare("UPDATE maintenance SET lastInvalidated = :lastInvalidated WHERE id = 1");
    query.bindValue(":lastInvalidated", QDate::currentDate().toString("yyyy-MM-dd"));

    if (not query.exec()) { return qApp->enqueueError(false, "Erro atualizando lastInvalidated: " + query.lastError().text()); }
  }

  return true;
}

void Application::startSqlPing() {
  auto *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [] { QSqlQuery().exec("DO 0"); });
  timer->start(60000);

  // TODO: se ping falhar marcar 'desconectado'?

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

  darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::HighlightedText, Qt::black);

  darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));
  darkPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(120, 120, 120));
  darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120));

  qApp->setPalette(darkPalette);

  qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

  UserSession::setSetting("User/tema", "escuro");
}

void Application::lightTheme() {
  qApp->setStyle("Fusion");
  qApp->setPalette(defaultPalette);
  qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

  UserSession::setSetting("User/tema", "claro");
}

bool Application::startTransaction(const bool delayMessages) {
  if (inTransaction) { return qApp->enqueueError(false, "Transação já em execução!"); }

  if (QSqlQuery query; not query.exec("START TRANSACTION")) { return qApp->enqueueError(false, "Erro iniciando transaction: " + query.lastError().text()); }

  inTransaction = true;
  this->delayMessages = delayMessages;

  return true;
}

bool Application::endTransaction() {
  if (not inTransaction) { return qApp->enqueueError(false, "Não está em transação"); }

  if (QSqlQuery query; not query.exec("COMMIT")) { return qApp->enqueueError(false, "Erro no commit: " + query.lastError().text()); }

  inTransaction = false;
  delayMessages = false;

  showMessages();

  return true;
}

void Application::rollbackTransaction() {
  if (not inTransaction) { return qApp->enqueueError("Não está em transação!"); }

  if (QSqlQuery query; not query.exec("ROLLBACK")) { return qApp->enqueueError("Erro rollback: " + query.lastError().text()); }

  QSqlQuery("ROLLBACK").exec();

  inTransaction = false;
  delayMessages = false;

  showMessages();
}

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

  if (not updating) { showMessages(); }
}

bool Application::getUpdating() const { return updating; }

void Application::showMessages() {
  if (delayMessages and (inTransaction or updating)) { return; }
  if (errorQueue.isEmpty() and warningQueue.isEmpty() and informationQueue.isEmpty()) { return; }

  showingErrors = true;

  // TODO: deal with 'Lost connection to MySQL server'

  for (const auto &error : std::as_const(errorQueue)) { QMessageBox::critical(error.widget, "Erro!", error.message); }
  for (const auto &warning : std::as_const(warningQueue)) { QMessageBox::warning(warning.widget, "Aviso!", warning.message); }
  for (const auto &information : std::as_const(informationQueue)) { QMessageBox::information(information.widget, "Informação!", information.message); }

  errorQueue.clear();
  warningQueue.clear();
  informationQueue.clear();

  showingErrors = false;
}

void Application::updater() {
  const auto hostname = UserSession::getSetting("Login/hostname");

  if (not hostname) { return; }

  auto *updater = new QSimpleUpdater(this);
  updater->setApplicationVersion(qApp->applicationVersion());
  updater->setReferenceUrl("http://" + hostname.value().toString() + "/versao.txt");
  updater->setDownloadUrl("http://" + hostname.value().toString() + "/Instalador.exe");
  updater->setSilent(true);
  updater->setShowNewestVersionMessage(true);
  updater->checkForUpdates();
}
