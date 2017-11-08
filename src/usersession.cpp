#include <QDate>
#include <QMessageBox>
#include <QSqlError>

#include "usersession.h"

int UserSession::idLoja() { return (query->value("idLoja").toInt()); }

int UserSession::idUsuario() { return (query->value("idUsuario").toInt()); }

QString UserSession::nome() { return (query->value("nome").toString()); }

bool UserSession::login(const QString &user, const QString &password, Tipo tipo) {
  if (tipo == Tipo::Autorizacao) {
    QSqlQuery queryAutorizar;
    queryAutorizar.prepare("SELECT idLoja, idUsuario, nome, tipo FROM usuario WHERE user = :user AND passwd = PASSWORD(:password) AND desativado = FALSE AND (tipo LIKE '%GERENTE%' OR tipo = "
                           "'ADMINISTRADOR' OR tipo = 'DIRETOR')");
    queryAutorizar.bindValue(":user", user);
    queryAutorizar.bindValue(":password", password);

    if (not queryAutorizar.exec()) {
      QMessageBox::critical(nullptr, "Erro!", "Erro no login: " + queryAutorizar.lastError().text());
      return false;
    }

    return queryAutorizar.first();
  }

  initialize();

  query->prepare("SELECT idLoja, idUsuario, nome, tipo FROM usuario WHERE user = :user AND passwd = PASSWORD(:password) AND desativado = FALSE");
  query->bindValue(":user", user);
  query->bindValue(":password", password);

  if (not query->exec()) {
    QMessageBox::critical(nullptr, "Erro!", "Erro no login: " + query->lastError().text());
    return false;
  }

  return query->first();
}

QString UserSession::tipoUsuario() { return (query->value("tipo").toString()); }

QString UserSession::fromLoja(const QString &parameter, const QString &user) {
  QSqlQuery queryLoja;
  queryLoja.prepare("SELECT " + parameter + " FROM loja LEFT JOIN usuario ON loja.idLoja = usuario.idLoja WHERE usuario.nome = :nome");
  queryLoja.bindValue(":nome", user);

  if (not queryLoja.exec() or not queryLoja.first()) {
    QMessageBox::critical(nullptr, "Erro!", "Erro na query loja: " + queryLoja.lastError().text());
    return QString();
  }

  return queryLoja.value(0).toString();
}

QVariant UserSession::getSetting(const QString &key) { return settings->value(key); }

void UserSession::setSetting(const QString &key, const QVariant &value) { settings->setValue(key, value); }

bool UserSession::settingsContains(const QString &key) { return settings->contains(key); }

void UserSession::initialize() {
  if (not query) {
    if (not dbConnect()) return;

    query = new QSqlQuery();
  }
}

bool UserSession::dbConnect() {
  if (not QSqlDatabase::drivers().contains("QMYSQL")) {
    QMessageBox::critical(nullptr, "Não foi possível carregar o banco de dados.", "Este aplicativo requer o driver QMYSQL.");
    exit(1);
  }

  QSqlDatabase db = QSqlDatabase::contains() ? QSqlDatabase::database() : QSqlDatabase::addDatabase("QMYSQL");

  db.setHostName(UserSession::getSetting("Login/hostname").toString());
  db.setUserName(UserSession::getSetting("User/lastuser").toString().toLower());
  db.setPassword("1234");
  db.setDatabaseName("mysql");

  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_RECONNECT=1");
  //  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_RECONNECT=1;MYSQL_OPT_CONNECT_TIMEOUT=60;MYSQL_OPT_READ_TIMEOUT=60;"
  //                       "MYSQL_OPT_WRITE_TIMEOUT=60");

  if (not db.open()) {
    QMessageBox::critical(nullptr, "Erro: Banco de dados inacessível!", db.lastError().nativeErrorCode());
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
