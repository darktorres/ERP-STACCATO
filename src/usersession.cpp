#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

#include "application.h"
#include "usersession.h"

int UserSession::idLoja() { return (query->value("idLoja").toInt()); }

int UserSession::idUsuario() { return (query->value("idUsuario").toInt()); }

QString UserSession::nome() { return (query->value("nome").toString()); }

bool UserSession::login(const QString &user, const QString &password, Tipo tipo) {
  if (tipo == Tipo::Autorizacao) {
    QSqlQuery queryAutorizar;
    queryAutorizar.prepare("SELECT idLoja, idUsuario, nome, tipo FROM usuario WHERE user = :user AND passwd = PASSWORD(:password) AND desativado = FALSE AND (tipo LIKE '%GERENTE%' OR tipo IN "
                           "('ADMINISTRADOR', 'DIRETOR'))");
    queryAutorizar.bindValue(":user", user);
    queryAutorizar.bindValue(":password", password);

    if (not queryAutorizar.exec()) { return qApp->enqueueError(false, "Erro no login: " + queryAutorizar.lastError().text()); }

    return queryAutorizar.first();
  }

  initializeQuery(); // TODO: move this to top of function?

  query->prepare("SELECT idLoja, idUsuario, nome, tipo FROM usuario WHERE user = :user AND passwd = PASSWORD(:password) AND desativado = FALSE");
  query->bindValue(":user", user);
  query->bindValue(":password", password);

  if (not query->exec()) { return qApp->enqueueError(false, "Erro no login: " + query->lastError().text()); }

  return query->first();
}

QString UserSession::tipoUsuario() { return (query->value("tipo").toString()); }

std::optional<QVariant> UserSession::fromLoja(const QString &parameter, const QString &user) {
  QSqlQuery queryLoja;
  queryLoja.prepare("SELECT " + parameter + " FROM loja LEFT JOIN usuario ON loja.idLoja = usuario.idLoja WHERE usuario.nome = :nome");
  queryLoja.bindValue(":nome", user);

  if (not queryLoja.exec() or not queryLoja.first()) {
    qApp->enqueueError("Erro na query loja: " + queryLoja.lastError().text());
    return {};
  }

  if (queryLoja.value(0).isNull()) { return {}; }

  return queryLoja.value(0);
}

std::optional<QVariant> UserSession::getSetting(const QString &key) {
  const auto value = settings->value(key);

  if (value.isNull()) { return {}; }
  if (value.type() == QVariant::String and value.toString().isEmpty()) { return {}; }

  return settings->value(key);
}

void UserSession::setSetting(const QString &key, const QVariant &value) { settings->setValue(key, value); }

void UserSession::initializeQuery() {
  if (not query) { query = new QSqlQuery(); }
}
