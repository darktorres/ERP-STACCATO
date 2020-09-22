#include "usersession.h"

#include "application.h"

#include <QSqlError>

QVariant UserSession::getSetting(const QString &key) { return settings->value(key); }

void UserSession::setSetting(const QString &key, const QVariant &value) { settings->setValue(key, value); }

void UserSession::login(const QString &user, const QString &password) {
  if (not query) { query = new SqlQuery(); }

  query->prepare("SELECT idLoja, idUsuario, nome, tipo FROM usuario WHERE user = :user AND passwd = PASSWORD(:password) AND desativado = FALSE");
  query->bindValue(":user", user);
  query->bindValue(":password", password);

  if (not query->exec()) { throw RuntimeException("Erro no login: " + query->lastError().text()); }

  if (not query->first()) { throw RuntimeError("Login inválido!"); }

  idLoja = query->value("idLoja").toInt();
  idUsuario = query->value("idUsuario").toInt();
  nome = query->value("nome").toString();
  tipoUsuario = query->value("tipo").toString();
}

void UserSession::autorizacao(const QString &user, const QString &password) {
  SqlQuery queryAutorizar;
  queryAutorizar.prepare("SELECT idLoja, idUsuario, nome, tipo FROM usuario WHERE user = :user AND passwd = PASSWORD(:password) AND desativado = FALSE AND "
                         "(tipo IN ('ADMINISTRADOR', 'ADMINISTRATIVO', 'DIRETOR', 'GERENTE DEPARTAMENTO', 'GERENTE LOJA'))");
  queryAutorizar.bindValue(":user", user);
  queryAutorizar.bindValue(":password", password);

  if (not queryAutorizar.exec()) { throw RuntimeException("Erro no login: " + queryAutorizar.lastError().text()); }

  if (not queryAutorizar.first()) { throw RuntimeError("Login inválido!"); }
}

QVariant UserSession::fromLoja(const QString &parameter, const QString &user) {
  SqlQuery queryLoja;
  queryLoja.prepare("SELECT " + parameter + " FROM loja LEFT JOIN usuario ON loja.idLoja = usuario.idLoja WHERE usuario.nome = :nome");
  queryLoja.bindValue(":nome", user);

  if (not queryLoja.exec() or not queryLoja.first()) { throw RuntimeException("Erro na query loja: " + queryLoja.lastError().text()); }

  return queryLoja.value(0);
}
