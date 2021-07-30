#include "user.h"

#include "application.h"

#include <QSqlError>

QVariant User::getSetting(const QString &key) { return settings->value(key); }

bool User::isAdmin() { return (User::tipo == "ADMINISTRADOR" or User::tipo == "DIRETOR"); }

bool User::isAdministrativo() { return (User::tipo == "ADMINISTRADOR" or User::tipo == "ADMINISTRATIVO" or User::tipo == "DIRETOR"); }

bool User::isEspecial() { return (User::tipo == "VENDEDOR ESPECIAL"); }

bool User::isGerente() { return (User::tipo == "GERENTE LOJA"); }

bool User::isOperacional() { return (User::tipo == "OPERACIONAL"); }

bool User::isVendedor() { return (User::tipo == "VENDEDOR"); }

bool User::isVendedorOrEspecial() { return (User::tipo == "VENDEDOR" or User::tipo == "VENDEDOR ESPECIAL"); }

void User::setSetting(const QString &key, const QVariant &value) { settings->setValue(key, value); }

void User::login(const QString &user, const QString &password) {
  if (not query) { query = new SqlQuery(); }

  query->prepare("SELECT idLoja, idUsuario, nome, tipo FROM usuario WHERE user = :user AND password = SHA_PASSWORD(:password) AND desativado = FALSE");
  query->bindValue(":user", user);
  query->bindValue(":password", password);

  if (not query->exec()) { throw RuntimeException("Erro no login: " + query->lastError().text()); }

  if (not query->first()) { throw RuntimeError("Login inválido!"); }

  idLoja = query->value("idLoja").toString();
  idUsuario = query->value("idUsuario").toString();
  nome = query->value("nome").toString();
  tipo = query->value("tipo").toString();
  usuario = user;
  senha = password;
}

void User::autorizacao(const QString &user, const QString &password) {
  SqlQuery queryAutorizar;
  queryAutorizar.prepare("SELECT idLoja, idUsuario, nome, tipo FROM usuario WHERE user = :user AND password = SHA_PASSWORD(:password) AND desativado = FALSE AND "
                         "(tipo IN ('ADMINISTRADOR', 'ADMINISTRATIVO', 'DIRETOR', 'GERENTE DEPARTAMENTO', 'GERENTE LOJA'))");
  queryAutorizar.bindValue(":user", user);
  queryAutorizar.bindValue(":password", password);

  if (not queryAutorizar.exec()) { throw RuntimeException("Erro no login: " + queryAutorizar.lastError().text()); }

  if (not queryAutorizar.first()) { throw RuntimeError("Login inválido!"); }
}

QVariant User::fromLoja(const QString &parameter, const QString &user) {
  SqlQuery queryLoja;
  queryLoja.prepare("SELECT " + parameter + " FROM loja LEFT JOIN usuario ON loja.idLoja = usuario.idLoja WHERE usuario.nome = :nome");
  queryLoja.bindValue(":nome", user);

  if (not queryLoja.exec()) { throw RuntimeException("Erro na query loja: " + queryLoja.lastError().text()); }

  if (not queryLoja.first()) { throw RuntimeException("Dados da loja/usuário não encontrados para o usuário: " + user); }

  return queryLoja.value(0);
}
