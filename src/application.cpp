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

RuntimeException::RuntimeException(const QString &message, QWidget *parent) : std::runtime_error(message.toStdString()) { qApp->enqueueException(message, parent); }

RuntimeError::RuntimeError(const QString &message, QWidget *parent) : std::runtime_error(message.toStdString()) { qApp->enqueueError(message, parent); }

Application::Application(int &argc, char **argv, int) : QApplication(argc, argv) {
  setOrganizationName("Staccato");
  setApplicationName("ERP");
  setWindowIcon(QIcon("Staccato.ico"));
  setApplicationVersion("0.9.8");
  setStyle("Fusion");

  readSettingsFile();

  storeSelection();

  if (UserSession::getSetting("User/tema").toString() == "escuro") { darkTheme(); }

  if (not QSqlDatabase::drivers().contains("QMYSQL")) {
    QMessageBox::critical(nullptr, "Erro!", "Este aplicativo requer o driver QMYSQL!");
    exit(1);
  }
}

// for system errors
void Application::enqueueException(const QString &exception, QWidget *parent) {
  // TODO: guardar o arquivo/linha que chamou essa funcao
  exceptionQueue << Message{exception, parent};

  Log::createLog("Exceção", exception);

  showMessages();
}

bool Application::enqueueException(const bool boolean, const QString &exception, QWidget *parent) {
  enqueueException(exception, parent);
  return boolean;
}

// for user errors
void Application::enqueueError(const QString &error, QWidget *parent) {
  errorQueue << Message{error, parent};

  Log::createLog("Erro", error);

  showMessages();
}

bool Application::enqueueError(const bool boolean, const QString &error, QWidget *parent) {
  enqueueError(error, parent);
  return boolean;
}

void Application::enqueueInformation(const QString &information, QWidget *parent) {
  informationQueue << Message{information, parent};

  showMessages();
}

void Application::enqueueWarning(const QString &warning, QWidget *parent) {
  warningQueue << Message{warning, parent};

  showMessages();
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

void Application::userLogin(const QString &user) {
  db.close();

  db.setUserName(user);

  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_CONNECT_TIMEOUT=3");
  //  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_READ_TIMEOUT=10;MYSQL_OPT_WRITE_TIMEOUT=10;MYSQL_OPT_CONNECT_TIMEOUT=3");

  if (not db.open()) { loginError(); }
}

void Application::genericLogin(const QString &hostname) {
  QFile file("mysql.txt");

  if (not file.open(QFile::ReadOnly)) { throw RuntimeException("Erro lendo mysql.txt: " + file.errorString()); }

  const QString systemPassword = file.readAll();

  // ------------------------------------------------------------

  if (not QSqlDatabase::contains()) { db = QSqlDatabase::addDatabase("QMYSQL"); }

  db.setHostName(hostname);
  db.setUserName("loginUser");
  db.setPassword(systemPassword);
  db.setDatabaseName("staccato");
  db.setPort(3306);

  db.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=1");

  if (not db.open()) {
    bool connected = false;

    for (const auto &loja : mapLojas) {
      db.setHostName(loja);

      if (db.open()) {
        UserSession::setSetting("Login/hostname", loja);
        UserSession::setSetting("Login/loja", mapLojas.key(loja));
        updater();
        connected = true;
        break;
      }
    }

    if (not connected) { loginError(); }
  }
}

void Application::loginError() {
  isConnected = false;

  const QString error = db.lastError().text();

  QString message = "Erro conectando no banco de dados: " + error;

  if (error.contains("Access denied for user")) { message = "Login inválido!"; }
  if (error.contains("Can't connect to MySQL server on")) { message = "Não foi possível conectar ao servidor!"; }

  throw RuntimeException(message);
}

bool Application::dbReconnect(const bool silent) {
  db.close();

  isConnected = db.open();

  if (not isConnected) {
    // TODO: tentar conectar nos outros servidores (codigo em genericLogin)
    if (not silent) { QMessageBox::critical(nullptr, "Erro!", "Erro conectando no banco de dados: " + db.lastError().text()); }
    return false;
  }

  return db.isOpen();
}

bool Application::dbConnect(const QString &hostname, const QString &user, const QString &userPassword) {
  genericLogin(hostname);

  UserSession::login(user, userPassword);

  userLogin(user);

  // ------------------------------------------------------------

  isConnected = true;

  runSqlJobs();

  startSqlPing();
  startUpdaterPing();

  return true;
}

void Application::runSqlJobs() {
  SqlQuery query;

  if (not query.exec("SELECT lastInvalidated FROM maintenance") or not query.first()) { throw RuntimeException("Erro verificando lastInvalidated: " + query.lastError().text()); }

  if (query.value("lastInvalidated").toDate() < serverDateTime().date()) {
    if (not query.exec("CALL invalidar_produtos_expirados()")) { throw RuntimeException("Erro executando invalidar_produtos_expirados: " + query.lastError().text()); }

    if (not query.exec("CALL invalidar_orcamentos_expirados()")) { throw RuntimeException("Erro executando invalidar_orcamentos_expirados: " + query.lastError().text()); }

    if (not query.exec("CALL invalidar_staccatoOff()")) { throw RuntimeException("Erro executando invalidar_staccatoOff: " + query.lastError().text()); }

    query.prepare("UPDATE maintenance SET lastInvalidated = :lastInvalidated WHERE id = 1");
    query.bindValue(":lastInvalidated", serverDateTime().toString("yyyy-MM-dd"));

    if (not query.exec()) { throw RuntimeException("Erro atualizando lastInvalidated: " + query.lastError().text()); }
  }
}

void Application::startSqlPing() {
  auto timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [] { QSqlQuery().exec("DO 0"); });
  timer->start(60000);

  // TODO: se ping falhar marcar 'desconectado'?
}

void Application::startUpdaterPing() {
  auto timer = new QTimer(this);
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

void Application::startTransaction(const QString &messageLog) {
  qDebug() << "startTransaction: " << messageLog;
  if (inTransaction) { throw RuntimeException("Transação já em execução!"); }

  if (SqlQuery query; not query.exec("START TRANSACTION")) { return; }

  Log::createLog("Transação", messageLog);

  inTransaction = true;
}

void Application::endTransaction() {
  qDebug() << "endTransaction";
  if (not inTransaction) { throw RuntimeException("Não está em transação"); }

  if (SqlQuery query; not query.exec("COMMIT")) { return; }

  inTransaction = false;

  showMessages();
}

void Application::rollbackTransaction() {
  qDebug() << "rollbackTransaction";
  if (inTransaction) {
    if (SqlQuery query; not query.exec("ROLLBACK")) { return; }
    inTransaction = false;
  }

  showMessages();
}

bool Application::getShowingMessages() const { return showingMessages; }

bool Application::getIsConnected() const { return isConnected; }

QMap<QString, QString> Application::getMapLojas() const { return mapLojas; }

void Application::storeSelection() {
  if (UserSession::getSetting("Login/hostname").toString().isEmpty()) {
    const QStringList items = mapLojas.keys();

    const QString loja = QInputDialog::getItem(nullptr, "Escolha a loja", "Qual a sua loja?", items, 0, false);

    if (loja.isEmpty()) { return; }

    UserSession::setSetting("Login/hostname", mapLojas.value(loja));
  }
}

void Application::setUpdating(const bool value) {
  updating = value;

  showMessages();
}

bool Application::getUpdating() const { return updating; }

void Application::showMessages() {
  if (inTransaction) { return; }
  if (updating) { return; }
  if (exceptionQueue.isEmpty() and errorQueue.isEmpty() and warningQueue.isEmpty() and informationQueue.isEmpty()) { return; }

  showingMessages = true;

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

    if (not silent) { QMessageBox::critical(exception.widget, "Erro!", exception.message); }
  }

  if (not silent) {
    for (const auto &error : std::as_const(errorQueue)) { QMessageBox::critical(error.widget, "Erro!", error.message); }
    for (const auto &warning : std::as_const(warningQueue)) { QMessageBox::warning(warning.widget, "Aviso!", warning.message); }
    for (const auto &information : std::as_const(informationQueue)) { QMessageBox::information(information.widget, "Informação!", information.message); }
  }

  exceptionQueue.clear();
  errorQueue.clear();
  warningQueue.clear();
  informationQueue.clear();

  showingMessages = false;
}

void Application::updater() {
  if (updaterOpen) { return; }

  const QString hostname = UserSession::getSetting("Login/hostname").toString();

  if (hostname.isEmpty()) { return; }

  updaterOpen = true;

  auto *updater = new QSimpleUpdater(this);
  connect(updater, &QSimpleUpdater::done, [&] { updaterOpen = false; });
  updater->setApplicationVersion(applicationVersion());
  updater->setReferenceUrl("http://" + hostname + "/versao.txt");
  updater->setDownloadUrl("http://" + hostname + "/Instalador.exe");
  updater->setSilent(true);
  updater->setShowNewestVersionMessage(true);
  updater->checkForUpdates();
}

bool Application::getSilent() const { return silent; }

void Application::setSilent(bool value) {
  qDebug() << "setSilent: " << value;
  silent = value;
}

bool Application::getInTransaction() const { return inTransaction; }

QDateTime Application::serverDateTime() {
  SqlQuery query;

  if (not query.exec("SELECT NOW()") or not query.first()) {
    enqueueException("Erro buscando data/hora: " + query.lastError().text());
    return QDateTime();
  }

  return query.value("NOW()").toDateTime();
}

QDate Application::serverDate() {
  if (serverDateCache.isNull() or systemDate.daysTo(QDate::currentDate()) > 0) {
    SqlQuery query;

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

QString Application::sanitizeSQL(const QString &string) {
  QString sanitized = string;

  sanitized.remove("+").remove("@").remove(">").remove("<").remove("(").remove(")").remove("~").remove("*").remove("'").remove("\\");

  return sanitized;
}

int Application::reservarIdEstoque() {
  if (inTransaction) { throw RuntimeException("ALTER TABLE durante transação!"); }

  SqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'estoque'") or not query.first()) {
    throw RuntimeException("Erro reservar id estoque: " + query.lastError().text());
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE estoque auto_increment = " + QString::number(id + 1))) { throw RuntimeException("Erro reservar id estoque: " + query.lastError().text()); }

  return id;
}

int Application::reservarIdVendaProduto2() {
  if (inTransaction) { throw RuntimeException("ALTER TABLE durante transação!"); }

  SqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'venda_has_produto2'") or not query.first()) {
    throw RuntimeException("Erro reservar id venda: " + query.lastError().text());
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE venda_has_produto2 auto_increment = " + QString::number(id + 1))) { throw RuntimeException("Erro reservar id venda: " + query.lastError().text()); }

  return id;
}

int Application::reservarIdNFe() {
  if (inTransaction) { throw RuntimeException("ALTER TABLE durante transação!"); }

  SqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'nfe'") or not query.first()) {
    throw RuntimeException("Erro reservar id nfe: " + query.lastError().text());
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE nfe auto_increment = " + QString::number(id + 1))) { throw RuntimeException("Erro reservar id nfe: " + query.lastError().text()); }

  return id;
}

int Application::reservarIdPedido2() {
  if (inTransaction) { throw RuntimeException("ALTER TABLE durante transação!"); }

  SqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'pedido_fornecedor_has_produto2'") or not query.first()) {
    throw RuntimeException("Erro reservar id compra: " + query.lastError().text());
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE pedido_fornecedor_has_produto2 auto_increment = " + QString::number(id + 1))) { throw RuntimeException("Erro reservar id compra: " + query.lastError().text()); }

  return id;
}

QDate Application::ajustarDiaUtil(const QDate &date) {
  // TODO: adicionar feriados bancarios

  QDate newDate = date;

  while (newDate.dayOfWeek() > 5) { newDate = newDate.addDays(1); }

  return newDate;
}

bool Application::notify(QObject *receiver, QEvent *event) {
  bool done = true;

  try {
    done = QApplication::notify(receiver, event);
  } catch (const std::exception &e) {
    qDebug() << "catch all: " << e.what();
    rollbackTransaction();
  }

  return done;
}
