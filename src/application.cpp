#include "application.h"

#include "log.h"
#include "qsimpleupdater.h"
#include "usersession.h"

#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QTimer>

Application::Application(int &argc, char **argv, int) : QApplication(argc, argv) {
  setOrganizationName("Staccato");
  setApplicationName("ERP");
  setWindowIcon(QIcon("Staccato.ico"));
  setApplicationVersion("0.8.120");
  setStyle("Fusion");

  readSettingsFile();

  storeSelection();

  if (UserSession::getSetting("User/tema").value_or("claro").toString() == "escuro") { darkTheme(); }

  if (not QSqlDatabase::drivers().contains("QMYSQL")) {
    QMessageBox::critical(nullptr, "Erro!", "Este aplicativo requer o driver QMYSQL!");
    exit(1);
  }
}

// for system errors
void Application::enqueueException(const QString &error, QWidget *parent) {
  // TODO: guardar o arquivo/linha que chamou essa funcao
  exceptionQueue << Message{error, parent};

  Log::createLog("Exceção", error, true);

  if (not updating) { showMessages(); }
}

bool Application::enqueueException(const bool boolean, const QString &error, QWidget *parent) {
  enqueueException(error, parent);
  return boolean;
}

// for user errors
void Application::enqueueError(const QString &error, QWidget *parent) {
  errorQueue << Message{error, parent};

  Log::createLog("Erro", error, true);

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

QString Application::getWebDavIp() const { return mapLojas.value("Acesso Externo - Alphaville"); }

void Application::readSettingsFile() {
  QFile file("lojas.txt");

  if (not file.open(QFile::ReadOnly)) {
    QMessageBox::critical(nullptr, "Erro!", "Erro lendo configurações: " + file.errorString());
    return;
  }

  const QStringList lines = QString(file.readAll()).split("\r\n", Qt::SkipEmptyParts);

  for (int i = 0; i < lines.size(); i += 2) { mapLojas.insert(lines.at(i), lines.at(i + 1)); }
}

bool Application::userLogin(const QString &user) {
  db.close();

  db.setUserName(user);

  setConnectOptions();

  if (not db.open()) {
    loginError();

    return false;
  }

  return true;
}

bool Application::genericLogin(const QString &hostname) {
  QFile file("mysql.txt");

  if (not file.open(QFile::ReadOnly)) {
    QMessageBox::critical(nullptr, "Erro!", "Erro lendo mysql.txt: " + file.errorString());
    return false;
  }

  const QString systemPassword = file.readAll();

  // ------------------------------------------------------------

  if (not QSqlDatabase::contains()) { db = QSqlDatabase::addDatabase("QMYSQL"); }

  db.setHostName(hostname);
  db.setUserName("loginUser");
  db.setPassword(systemPassword);
  db.setDatabaseName("staccato");
  db.setPort(3306);

  setConnectOptions();

  if (not db.open()) {
    loginError();

    return false;
  }

  return true;
}

void Application::loginError() {
  // TODO: caso o servidor selecionado nao esteja disponivel tente os outros
  // TODO: try local ip's first
  isConnected = false;

  const QString error = db.lastError().text();

  QString message = "Erro conectando no banco de dados: " + error;

  if (error.contains("Access denied for user")) { message = "Login inválido!"; }
  if (error.contains("Can't connect to MySQL server on")) { message = "Não foi possível conectar ao servidor!"; }

  QMessageBox::critical(nullptr, "Erro!", message);
}

void Application::setConnectOptions() {
  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_CONNECT_TIMEOUT=3");
  //  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_READ_TIMEOUT=10;MYSQL_OPT_WRITE_TIMEOUT=10;MYSQL_OPT_CONNECT_TIMEOUT=3");
}

bool Application::dbReconnect(const bool silent) {
  db.close();

  isConnected = db.open();

  if (not isConnected) {
    if (not silent) { QMessageBox::critical(nullptr, "Erro!", "Erro conectando no banco de dados: " + db.lastError().text()); }
    return false;
  }

  return db.isOpen();
}

bool Application::dbConnect(const QString &hostname, const QString &user, const QString &userPassword) {
  if (not genericLogin(hostname)) { return false; }

  if (not UserSession::login(user, userPassword)) {
    QMessageBox::critical(nullptr, "Erro!", "Login inválido!");
    return false;
  }

  if (not userLogin(user)) { return false; }

  // ------------------------------------------------------------

  isConnected = true;

  if (not runSqlJobs()) { return false; }

  startSqlPing();
  startUpdaterPing();

  return true;
}

bool Application::runSqlJobs() {
  QSqlQuery query;

  if (not query.exec("SELECT lastInvalidated FROM maintenance") or not query.first()) {
    QMessageBox::critical(nullptr, "Erro!", "Erro verificando lastInvalidated: " + query.lastError().text());
    return false;
  }

  if (query.value("lastInvalidated").toDate() < serverDateTime().date()) {
    if (not query.exec("CALL invalidar_produtos_expirados()")) {
      QMessageBox::critical(nullptr, "Erro!", "Erro executando invalidar_produtos_expirados: " + query.lastError().text());
      return false;
    }

    if (not query.exec("CALL invalidar_orcamentos_expirados()")) {
      QMessageBox::critical(nullptr, "Erro!", "Erro executando invalidar_orcamentos_expirados: " + query.lastError().text());
      return false;
    }

    if (not query.exec("CALL invalidar_staccatoOff()")) {
      QMessageBox::critical(nullptr, "Erro!", "Erro executando invalidar_staccatoOff: " + query.lastError().text());
      return false;
    }

    query.prepare("UPDATE maintenance SET lastInvalidated = :lastInvalidated WHERE id = 1");
    query.bindValue(":lastInvalidated", serverDateTime().toString("yyyy-MM-dd"));

    if (not query.exec()) {
      QMessageBox::critical(nullptr, "Erro!", "Erro atualizando lastInvalidated: " + query.lastError().text());
      return false;
    }
  }

  return true;
}

void Application::startSqlPing() {
  auto *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [] { QSqlQuery().exec("DO 0"); });
  timer->start(60000);

  // TODO: se ping falhar marcar 'desconectado'?
}

void Application::startUpdaterPing() {
  auto *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [&] { updater(); });
  timer->start(600000);
}

void Application::darkTheme() {
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

  setPalette(darkPalette);

  setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

  UserSession::setSetting("User/tema", "escuro");
}

void Application::lightTheme() {
  setPalette(defaultPalette);
  setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

  UserSession::setSetting("User/tema", "claro");
}

bool Application::startTransaction(const QString &messageLog, const bool delayMessages) {
  if (inTransaction) {
    // TODO: this message wont show due to inTransaction flag (look for other places that need to use a messagebox directly)
    return qApp->enqueueException(false, "Transação já em execução!");
  }

  if (QSqlQuery query; not query.exec("START TRANSACTION")) { return enqueueException(false, "Erro iniciando transaction: " + query.lastError().text()); }

  if (not Log::createLog("Transação", messageLog)) { return rollbackTransaction(false); }

  inTransaction = true;
  this->delayMessages = delayMessages;

  return true;
}

bool Application::endTransaction() {
  if (not inTransaction) { return enqueueException(false, "Não está em transação"); }

  if (QSqlQuery query; not query.exec("COMMIT")) { return enqueueException(false, "Erro no commit: " + query.lastError().text()); }

  inTransaction = false;
  delayMessages = false;

  showMessages();

  return true;
}

void Application::rollbackTransaction() {
  if (not inTransaction) { return enqueueException("Não está em transação!"); }

  if (QSqlQuery query; not query.exec("ROLLBACK")) { return enqueueException("Erro rollback: " + query.lastError().text()); }

  inTransaction = false;
  delayMessages = false;

  showMessages();
}

bool Application::rollbackTransaction(const bool boolean) {
  rollbackTransaction();
  return boolean;
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

void Application::setUpdating(const bool value) {
  updating = value;

  if (not updating) { showMessages(); }
}

bool Application::getUpdating() const { return updating; }

void Application::showMessages() {
  if (delayMessages and (inTransaction or updating)) { return; }
  if (errorQueue.isEmpty() and warningQueue.isEmpty() and informationQueue.isEmpty() and exceptionQueue.isEmpty()) { return; }

  showingErrors = true;

  for (const auto &exception : std::as_const(exceptionQueue)) {
    const QString error1 = "MySQL server has gone away";
    const QString error2 = "Lost connection to MySQL server during query";

    const QString message = exception.message;

    if (message.contains(error1) or message.contains(error2)) {
      const bool conectado = dbReconnect(true);

      emit verifyDb(conectado);

      if (conectado) {
        continue;
      } else {
        QMessageBox::critical(exception.widget, "Erro!", "Conexão com o servidor perdida!");
      }
    }

    QMessageBox::critical(exception.widget, "Erro!", exception.message);
  }

  for (const auto &error : std::as_const(errorQueue)) { QMessageBox::critical(error.widget, "Erro!", error.message); }
  for (const auto &warning : std::as_const(warningQueue)) { QMessageBox::warning(warning.widget, "Aviso!", warning.message); }
  for (const auto &information : std::as_const(informationQueue)) { QMessageBox::information(information.widget, "Informação!", information.message); }

  exceptionQueue.clear();
  errorQueue.clear();
  warningQueue.clear();
  informationQueue.clear();

  showingErrors = false;
}

void Application::updater() {
  if (updaterOpen) { return; }

  const auto hostname = UserSession::getSetting("Login/hostname");

  if (not hostname) { return; }

  updaterOpen = true;

  // TODO: add timeout in Qt 5.15 to avoid waiting for non available hosts
  // (quickly changing hosts dont work as updater is still waiting for the first one to respond)

  auto *updater = new QSimpleUpdater(this);
  connect(updater, &QSimpleUpdater::done, [&] { updaterOpen = false; });
  updater->setApplicationVersion(applicationVersion());
  updater->setReferenceUrl("http://" + hostname->toString() + "/versao.txt");
  updater->setDownloadUrl("http://" + hostname->toString() + "/Instalador.exe");
  updater->setSilent(true);
  updater->setShowNewestVersionMessage(true);
  updater->checkForUpdates();
}

bool Application::getInTransaction() const { return inTransaction; }

QDateTime Application::serverDateTime() {
  QSqlQuery query;

  if (not query.exec("SELECT NOW()") or not query.first()) {
    enqueueException("Erro buscando data/hora: " + query.lastError().text());
    return QDateTime();
  }

  return query.value("NOW()").toDateTime();
}

QDate Application::serverDate() {
  if (serverDateCache.isNull() or systemDate.daysTo(QDate::currentDate()) > 0) {
    QSqlQuery query;

    if (not query.exec("SELECT NOW()") or not query.first()) {
      enqueueException("Erro buscando data/hora: " + query.lastError().text());
      return QDate();
    }

    systemDate = QDate::currentDate();
    serverDateCache = query.value("NOW()").toDateTime();
  }

  return serverDateCache.date();
}

double Application::roundDouble(const double value) { return roundDouble(value, 4); }

double Application::roundDouble(const double value, const int decimais) {
  const double multiploDez = std::pow(10, decimais);

  return std::round(value * multiploDez) / multiploDez;
}

std::optional<int> Application::reservarIdEstoque() {
  if (inTransaction) {
    enqueueException("ALTER TABLE durante transação!");
    return {};
  }

  QSqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'estoque'") or not query.first()) {
    enqueueException("Erro reservar id estoque: " + query.lastError().text());
    return {};
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE estoque auto_increment = " + QString::number(id + 1))) {
    enqueueException("Erro reservar id estoque: " + query.lastError().text());
    return {};
  }

  return id;
}

std::optional<int> Application::reservarIdVendaProduto2() {
  if (inTransaction) {
    enqueueException("ALTER TABLE durante transação!");
    return {};
  }

  QSqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'venda_has_produto2'") or not query.first()) {
    enqueueException("Erro reservar id venda: " + query.lastError().text());
    return {};
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE venda_has_produto2 auto_increment = " + QString::number(id + 1))) {
    enqueueException("Erro reservar id venda: " + query.lastError().text());
    return {};
  }

  return id;
}

std::optional<int> Application::reservarIdNFe() {
  if (inTransaction) {
    enqueueException("ALTER TABLE durante transação!");
    return {};
  }

  QSqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'nfe'") or not query.first()) {
    enqueueException("Erro reservar id nfe: " + query.lastError().text());
    return {};
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE nfe auto_increment = " + QString::number(id + 1))) {
    enqueueException("Erro reservar id nfe: " + query.lastError().text());
    return {};
  }

  return id;
}

std::optional<int> Application::reservarIdPedido2() {
  if (inTransaction) {
    enqueueException("ALTER TABLE durante transação!");
    return {};
  }

  QSqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'pedido_fornecedor_has_produto2'") or not query.first()) {
    enqueueException("Erro reservar id compra: " + query.lastError().text());
    return {};
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE pedido_fornecedor_has_produto2 auto_increment = " + QString::number(id + 1))) {
    enqueueException("Erro reservar id compra: " + query.lastError().text());
    return {};
  }

  return id;
}

QDate Application::ajustarDiaUtil(const QDate &date) {
  // TODO: adicionar feriados bancarios

  QDate newDate = date;

  while (newDate.dayOfWeek() > 5) { newDate = newDate.addDays(1); }

  return newDate;
}
