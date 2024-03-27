#include "application.h"

#include "QSimpleUpdater.h"
#include "acbrlib.h"
#include "estoque.h"
#include "file.h"
#include "inputdialogfinanceiro.h"
#include "log.h"
#include "user.h"
#include "venda.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSqlError>
#include <QTimer>
#include <cmath>

using namespace std::chrono_literals;

RuntimeException::RuntimeException(const QString &message, QWidget *parent) : std::runtime_error(message.toStdString()) { qApp->enqueueException(message, parent); }

RuntimeError::RuntimeError(const QString &message, QWidget *parent) : std::runtime_error(message.toStdString()) { qApp->enqueueError(message, parent); }

Application::Application(int &argc, char **argv) : QApplication(argc, argv) {
  setStyle("Fusion");

  QDir::setCurrent(QCoreApplication::applicationDirPath());

  readSettingsFile();

  if (User::getSetting("User/tema").toString() == "escuro") { darkTheme(); }

  if (not QSqlDatabase::drivers().contains("QMYSQL")) {
    QMessageBox::critical(nullptr, "Erro!", "Este aplicativo requer o driver QMYSQL!");
    exit(1);
  }
}

Application::~Application() { db.close(); }

// for system errors
void Application::enqueueException(const QString &exception, QWidget *parent) {
  // TODO: guardar o arquivo/linha que chamou essa funcao (stacktrace?)
  exceptionQueue << Message{exception, parent};

  showMessages();
}

// for user errors
void Application::enqueueError(const QString &error, QWidget *parent) {
  errorQueue << Message{error, parent};

  showMessages();
}

void Application::enqueueInformation(const QString &information, QWidget *parent) {
  informationQueue << Message{information, parent};

  showMessages();
}

void Application::enqueueWarning(const QString &warning, QWidget *parent) {
  warningQueue << Message{warning, parent};

  showMessages();
}

QString Application::getWebDavIp() const { return mapLojas.value("Acesso Externo - Locaweb"); }

void Application::readSettingsFile() {
  File file("lojas.txt");

  if (not file.open(QFile::ReadOnly)) {
    QMessageBox::critical(nullptr, "Erro!", "Erro lendo configurações: " + file.errorString());
    return;
  }

  QString fileContent = file.readAll();

  if (fileContent.isEmpty()) { throw RuntimeException("lojas.txt vazio!"); }

  fileContent.replace("\r\n", "\n"); // so it works with both Windows and Unix line endings

  const QStringList lines = fileContent.split("\n", Qt::SkipEmptyParts);

  for (int i = 0; i < lines.size(); i += 2) { mapLojas.insert(lines.at(i), lines.at(i + 1)); }
}

QString Application::googleMapsApi() {
  File file("google_api.txt");

  if (not file.open(QFile::ReadOnly)) { throw RuntimeException("Erro lendo chave da API: " + file.errorString()); }

  QString fileContent = file.readAll();

  if (fileContent.isEmpty()) { throw RuntimeException("google_api.txt vazio!"); }

  return fileContent;
}

void Application::userLogin(const QString &user) {
  db.close();

  db.setUserName(user);

  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_CONNECT_TIMEOUT=3");
  //  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_READ_TIMEOUT=10;MYSQL_OPT_WRITE_TIMEOUT=10;MYSQL_OPT_CONNECT_TIMEOUT=3");

  if (not db.open()) { loginError(); }
}

void Application::genericLogin(const QString &hostname) {
  File file("mysql.txt");

  if (not file.open(QFile::ReadOnly)) { throw RuntimeException("Erro lendo mysql.txt: " + file.errorString()); }

  const QString systemPassword = file.readAll().trimmed();

  if (systemPassword.isEmpty()) { throw RuntimeException("mysql.txt vazio!"); }

  // ------------------------------------------------------------

  if (not QSqlDatabase::contains()) { db = QSqlDatabase::addDatabase("QMYSQL"); }

  db.setHostName(hostname);
  db.setUserName("loginUser");
  db.setPassword(systemPassword);
  db.setDatabaseName("staccato");
  //  db.setDatabaseName("empty"); // for testing
  db.setPort(3306);

  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_CONNECT_TIMEOUT=3");

  if (not db.open()) {
    bool connected = false;

    //    for (const auto &loja : qAsConst(mapLojas)) {
    //      db.setHostName(loja);

    //      if (db.open()) {
    //        User::setSetting("Login/hostname", loja);
    //        User::setSetting("Login/loja", mapLojas.key(loja));
    //        updater();
    //        connected = true;
    //        break;
    //      }
    //    }

    if (not connected) { loginError(); }
  }
}

void Application::loginError() {
  isConnected = false;

  throw RuntimeException(db.lastError().text());
}

void Application::dbReconnect(const bool isSilent) {
  db.close();

  isConnected = db.open();

  emit setConnectionStatus(isConnected);

  if (not isConnected) {
    // TODO: tentar conectar nos outros servidores (codigo em genericLogin)
    if (not isSilent) { QMessageBox::critical(nullptr, "Erro!", "Erro conectando no banco de dados: " + db.lastError().text()); }
  }
}

void Application::dbConnect(const QString &hostname, const QString &user, const QString &userPassword) {
  genericLogin(hostname);

  User::login(user, userPassword);

  if (User::isOperacional()) { throw RuntimeError("Operacional bloqueado!"); }

  userLogin(user);

  // ------------------------------------------------------------

  isConnected = true;

  runSqlJobs();

  startSqlPing();
}

void Application::runSqlJobs() {
  SqlQuery query;

  if (not query.exec("SELECT lastInvalidated FROM maintenance")) { throw RuntimeException("Erro verificando lastInvalidated: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Erro verificando lastInvalidated!"); }

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
  auto *timer = new QTimer(this);

  connect(timer, &QTimer::timeout, this, [&] {
    if (not QSqlQuery().exec("DO 0")) { dbReconnect(true); }
  });

  timer->start(1min);
}

void Application::darkTheme() {
  // https://github.com/randrew/phantomstyle
  // https://github.com/ColinDuquesnoy/QDarkStyleSheet

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

  User::setSetting("User/tema", "escuro");
}

void Application::lightTheme() {
  setPalette(defaultPalette);
  setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

  User::setSetting("User/tema", "claro");
}

void Application::startTransaction(const QString &messageLog) {
  if (inTransaction) { throw RuntimeException("Transação já em execução!"); }

  qDebug() << "startTransaction: " << messageLog;

  if (SqlQuery query; not query.exec("START TRANSACTION")) { throw RuntimeException("Erro iniciando transação!"); }

  Log::createLog("Transação", messageLog);

  inTransaction = true;
}

void Application::endTransaction() {
  // TODO: verificar se a transação ainda é válida
  // https://dba.stackexchange.com/questions/128726/transaction-identifier-possible-with-mysql

  if (not inTransaction) { throw RuntimeException("Não está em transação"); }

  qDebug() << "endTransaction";

  if (SqlQuery query; not query.exec("COMMIT")) { throw RuntimeException("Erro encerrando transação!"); }

  inTransaction = false;

  showMessages();
}

void Application::rollbackTransaction(const QString &message) {
  if (inTransaction) {
    qDebug() << "rollbackTransaction: " << message;
    SqlQuery query;
    query.exec("ROLLBACK");

    if (not message.isEmpty()) { Log::createLog("exceção", message); }

    inTransaction = false;
  }

  showMessages();
}

bool Application::getShowingMessages() const { return showingMessages; }

bool Application::getIsConnected() const { return isConnected; }

QMap<QString, QString> Application::getMapLojas() const { return mapLojas; }

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

  for (auto &exception : exceptionQueue) {
    Log::createLog("Exceção", exception.message);

    if (exception.message.contains("Access denied for user")) { exception.message = "Login inválido!"; }
    if (exception.message.contains("Can't connect to MySQL server")) { exception.message = "Não foi possível conectar ao servidor!"; }
    if (exception.message.contains("MySQL server has gone away")) { exception.message = "Conexão com o servidor perdida!"; }
    if (exception.message.contains("Lost connection to MySQL server during query")) { exception.message = "Conexão com o servidor perdida!"; }
    if (exception.message.contains("WSREP has not yet prepared node for application use")) { exception.message = "Servidor fora de sincronia! Aguarde um momento ou conecte-se em outro servidor!"; }
    if (exception.message.contains("WSREP detected deadlock/conflict and aborted the transaction")) { exception.message = "Conflito detectado no banco de dados! Tente novamente!"; }
    if (exception.message.contains("Lock wait timeout exceeded")) { exception.message = "Tabela em uso! Aguarde um momento e tente novamente!"; }
    if (exception.message.contains("Deadlock found when trying to get lock")) { exception.message = "Conflito detectado no banco de dados! Tente novamente!"; }
    if (exception.message.contains("Data too long for column")) { exception.message = "Erro no banco de dados! Entre em contato com o suporte!"; }
    if (exception.message.contains("a foreign key constraint fails")) { exception.message = "Erro no banco de dados! Entre em contato com o suporte!"; }
    if (exception.message.contains("Duplicate entry")) { exception.message = "Erro no banco de dados! Entre em contato com o suporte!"; }
    if (exception.message.contains("cannot be null")) { exception.message = "Erro no banco de dados! Entre em contato com o suporte!"; }
    if (exception.message.contains("Incorrect string value")) { exception.message = "Erro no banco de dados! Entre em contato com o suporte!"; }

    if (exception.message == "Conexão com o servidor perdida!") {
      isConnected = false;
      emit setConnectionStatus(false);
    }

    if (not silent) { QMessageBox::critical(exception.widget, "Erro!", exception.message); }
  }

  for (const auto &error : std::as_const(errorQueue)) {
    Log::createLog("Erro", error.message);

    if (not silent) { QMessageBox::critical(error.widget, "Erro!", error.message); }
  }

  for (const auto &warning : std::as_const(warningQueue)) {
    Log::createLog("Aviso", warning.message);

    if (not silent) { QMessageBox::warning(warning.widget, "Aviso!", warning.message); }
  }

  for (const auto &information : std::as_const(informationQueue)) {
    Log::createLog("Informação", information.message);

    if (not silent) { QMessageBox::information(information.widget, "Informação!", information.message); }
  }

  exceptionQueue.clear();
  errorQueue.clear();
  warningQueue.clear();
  informationQueue.clear();

  showingMessages = false;
}

void Application::updater() {
#ifdef Q_OS_WIN
  if (updaterOpen) { return; }

  const QString hostname = User::getSetting("Login/hostname").toString();

  if (hostname.isEmpty()) { return; }

  updaterOpen = true;

  // TODO: abrir updater em um processo separado e fechar erp?

  auto *updater = new QSimpleUpdater(this);
  connect(updater, &QSimpleUpdater::done, [&] { updaterOpen = false; });
  updater->setApplicationVersion(applicationVersion());
  updater->setReferenceUrl("http://" + hostname + "/versao.txt");
  updater->setDownloadUrl("http://" + hostname + "/Instalador.exe");
  updater->setSilent(true);
  updater->setShowNewestVersionMessage(true);
  updater->checkForUpdates();
#endif
}

QString Application::removerDiacriticos(const QString &s, const bool removerSimbolos) {
  QString normalized = s.normalized(QString::NormalizationForm_KD);
  QString result;

  copy_if(normalized.begin(), normalized.end(), std::back_inserter(result), [](QChar &c) { return c.toLatin1() != 0; });

  if (removerSimbolos) { result.remove(QRegularExpression("[^a-zA-Z\\d\\s]")); }

  return result;
}

bool Application::getSilent() const { return silent; }

void Application::setSilent(const bool value) { silent = value; }

bool Application::getInTransaction() const { return inTransaction; }

QDateTime Application::serverDateTime() {
  SqlQuery query;

  if (not query.exec("SELECT NOW()")) { throw RuntimeException("Erro buscando data/hora: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Erro buscando data/hora!"); }

  return query.value("NOW()").toDateTime();
}

QDate Application::serverDate() {
  if (serverDateCache.isNull() or systemDate.daysTo(QDate::currentDate()) > 0) {
    SqlQuery query;

    if (not query.exec("SELECT CURDATE()")) { throw RuntimeException("Erro buscando data/hora: " + query.lastError().text()); }

    if (not query.first()) { throw RuntimeException("Erro buscando data/hora!"); }

    systemDate = QDate::currentDate();
    serverDateCache = query.value("CURDATE()").toDate();
  }

  return serverDateCache;
}

double Application::roundDouble(const double value) { return roundDouble(value, 4); }

double Application::roundDouble(const double value, const int decimais) {
  const double multiploDez = std::pow(10, decimais);

  return std::round(value * multiploDez) / multiploDez;
}

QString Application::sanitizeSQL(const QString &string) {
  QString sanitized = string;

  sanitized.remove("+").remove("@").remove(">").remove("<").remove("~").remove("*").remove("'").remove(R"(\)");

  return sanitized;
}

int Application::reservarIdEstoque() {
  if (inTransaction) { throw RuntimeException("ALTER TABLE durante transação!"); }

  SqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = '" + db.databaseName() + "' AND table_name = 'estoque'")) {
    throw RuntimeException("Erro reservando idEstoque: " + query.lastError().text());
  }

  if (not query.first()) { throw RuntimeException("Erro reservando idEstoque!"); }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE estoque auto_increment = " + QString::number(id + 1))) { throw RuntimeException("Erro reservando idEstoque: " + query.lastError().text()); }

  return id;
}

int Application::reservarIdVendaProduto2() {
  if (inTransaction) { throw RuntimeException("ALTER TABLE durante transação!"); }

  SqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = '" + db.databaseName() + "' AND table_name = 'venda_has_produto2'")) {
    throw RuntimeException("Erro reservando idVenda: " + query.lastError().text());
  }

  if (not query.first()) { throw RuntimeException("Erro reservando idVenda!"); }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE venda_has_produto2 auto_increment = " + QString::number(id + 1))) { throw RuntimeException("Erro reservando idVenda: " + query.lastError().text()); }

  return id;
}

int Application::reservarIdNFe() {
  if (inTransaction) { throw RuntimeException("ALTER TABLE durante transação!"); }

  SqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = '" + db.databaseName() + "' AND table_name = 'nfe'")) {
    throw RuntimeException("Erro reservando idNFe: " + query.lastError().text());
  }

  if (not query.first()) { throw RuntimeException("Erro reservando idNFe!"); }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE nfe auto_increment = " + QString::number(id + 1))) { throw RuntimeException("Erro reservando idNFe: " + query.lastError().text()); }

  return id;
}

int Application::reservarIdPagamento() {
  if (inTransaction) { throw RuntimeException("ALTER TABLE durante transação!"); }

  SqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = '" + db.databaseName() + "' AND table_name = 'conta_a_pagar_has_pagamento'")) {
    throw RuntimeException("Erro reservando idPagamento: " + query.lastError().text());
  }

  if (not query.first()) { throw RuntimeException("Erro reservando idPagamento!"); }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE conta_a_pagar_has_pagamento auto_increment = " + QString::number(id + 1))) { throw RuntimeException("Erro reservando idPagamento: " + query.lastError().text()); }

  return id;
}

int Application::reservarIdPedido2() {
  if (inTransaction) { throw RuntimeException("ALTER TABLE durante transação!"); }

  SqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = '" + db.databaseName() + "' AND table_name = 'pedido_fornecedor_has_produto2'")) {
    throw RuntimeException("Erro reservando idPedido2: " + query.lastError().text());
  }

  if (not query.first()) { throw RuntimeException("Erro reservando idPedido2!"); }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE pedido_fornecedor_has_produto2 auto_increment = " + QString::number(id + 1))) { throw RuntimeException("Erro reservando idPedido2: " + query.lastError().text()); }

  return id;
}

QDate Application::ajustarDiaUtil(const QDate date) {
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
    rollbackTransaction(e.what());
  }

  return done;
}

QString Application::findTag(const QString &texto, const QString &tag) {
  const int index = texto.indexOf("\r\n" + tag, Qt::CaseInsensitive);

  if (index == -1) { throw RuntimeException("Não encontrou o campo '" + tag + "' no evento: " + texto); }

  return texto.mid(index + tag.length() + 2).split("\r\n").first();
}

void Application::abrirVenda(const QVariant &idVenda) {
  if (not idVenda.isValid() or idVenda.toString().isEmpty()) { return; }

  auto *venda = new Venda(nullptr);
  venda->setAttribute(Qt::WA_DeleteOnClose);
  venda->viewRegisterById(idVenda);
  venda->show();
}

void Application::abrirEstoque(const QVariant &idEstoque) {
  if (not idEstoque.isValid() or idEstoque.toString().isEmpty()) { return; }

  auto *estoque = new Estoque(idEstoque, nullptr);
  estoque->setAttribute(Qt::WA_DeleteOnClose);
  estoque->show();
}

void Application::abrirNFe(const QVariant &idNFe) {
  if (not idNFe.isValid() or idNFe.toString().isEmpty()) { return; }

  ACBrLib::gerarDanfe(idNFe.toInt());
}

void Application::abrirCompra(const QVariant &ordemCompra) {
  if (not ordemCompra.isValid() or ordemCompra.toString().isEmpty()) { return; }

  auto *input = new InputDialogFinanceiro(InputDialogFinanceiro::Tipo::Historico, nullptr);
  input->setAttribute(Qt::WA_DeleteOnClose);
  input->setFilter(ordemCompra.toString());
  input->show(); // TODO: porque esse abre em tela cheia se os outros abrem normal?
}
