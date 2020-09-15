#include "usersession.h"

#include "application.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

int UserSession::idLoja() { return (query->value("idLoja").toInt()); }

int UserSession::idUsuario() { return (query->value("idUsuario").toInt()); }

QString UserSession::nome() { return (query->value("nome").toString()); }

QString UserSession::tipoUsuario() { return (query->value("tipo").toString()); }

bool UserSession::login(const QString &user, const QString &password, LoginDialog::Tipo tipo) {
  if (tipo == LoginDialog::Tipo::Autorizacao) { // query separada para não alterar a query usada nas outras funções
    QSqlQuery queryAutorizar;
    queryAutorizar.prepare("SELECT idLoja, idUsuario, nome, tipo FROM usuario WHERE user = :user AND passwd = PASSWORD(:password) AND desativado = FALSE AND "
                           "(tipo IN ('ADMINISTRADOR', 'ADMINISTRATIVO', 'DIRETOR', 'GERENTE DEPARTAMENTO', 'GERENTE LOJA'))");
    queryAutorizar.bindValue(":user", user);
    queryAutorizar.bindValue(":password", password);

    if (not queryAutorizar.exec()) { throw RuntimeError("Erro no login: " + queryAutorizar.lastError().text()); }

    return queryAutorizar.first();
  }

  if (tipo == LoginDialog::Tipo::Login) {
    if (not query) { query = new QSqlQuery(); }

    query->prepare("SELECT idLoja, idUsuario, nome, tipo FROM usuario WHERE user = :user AND passwd = PASSWORD(:password) AND desativado = FALSE");
    query->bindValue(":user", user);
    query->bindValue(":password", password);

    if (not query->exec()) { throw RuntimeError("Erro no login: " + query->lastError().text()); }

    return query->first();
  }

  return false;
}

QVariant UserSession::fromLoja(const QString &parameter, const QString &user) {
  QSqlQuery queryLoja;
  queryLoja.prepare("SELECT " + parameter + " FROM loja LEFT JOIN usuario ON loja.idLoja = usuario.idLoja WHERE usuario.nome = :nome");
  queryLoja.bindValue(":nome", user);

  if (not queryLoja.exec() or not queryLoja.first()) { throw RuntimeError("Erro na query loja: " + queryLoja.lastError().text()); }

  return queryLoja.value(0);
}

QVariant UserSession::getSetting(const QString &key) { return settings->value(key); }

void UserSession::setSetting(const QString &key, const QVariant &value) { settings->setValue(key, value); }
