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

  initializeQuery();

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

// REFAC: make this return a optional
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

// REFAC: make this return a optional
QVariant UserSession::getSetting(const QString &key) {
  if (settings->value(key).isNull()) QMessageBox::critical(nullptr, "Erro!", "A chave " + key + " não está configurada!");

  return settings->value(key);
}

void UserSession::setSetting(const QString &key, const QVariant &value) { settings->setValue(key, value); }

bool UserSession::settingsContains(const QString &key) { return settings->contains(key); }

void UserSession::initializeQuery() {
  if (not query) query = new QSqlQuery();
}
