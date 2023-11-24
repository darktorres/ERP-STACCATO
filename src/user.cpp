#include "user.h"

#include "application.h"

#include <QSqlError>

QVariant User::getSetting(const QString &key) { return settings->value(key); }

bool User::isAdmin() { return (User::tipo == "ADMINISTRADOR" or User::tipo == "DIRETOR"); }

bool User::isAdministrativo() { return (User::tipo == "ADMINISTRADOR" or User::tipo == "ADMINISTRATIVO" or User::tipo == "DIRETOR"); }

bool User::isEspecial() { return (User::tipo == "VENDEDOR ESPECIAL"); }

// TODO: testar a adição dos gerentes dep./fin.
bool User::isGerente() { return (User::tipo == "GERENTE DEPARTAMENTO" or User::tipo == "GERENTE FINANCEIRO" or User::tipo == "GERENTE LOJA"); }

bool User::isOperacional() { return (User::tipo == "OPERACIONAL"); }

bool User::isVendedor() { return (User::tipo == "VENDEDOR"); }

bool User::isVendedorOrEspecial() { return (User::tipo == "VENDEDOR" or User::tipo == "VENDEDOR ESPECIAL"); }

void User::setSetting(const QString &key, const QVariant &value) { settings->setValue(key, value); }

bool User::temPermissao(const QString &permissao) {
  if (permissao.isEmpty()) { throw RuntimeException("Erro na consulta de permissão!"); }

  SqlQuery query;

  if (not query.exec("SELECT " + permissao + " FROM usuario_has_permissao WHERE idUsuario = " + idUsuario)) { throw RuntimeException("Erro lendo permissões: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Permissões não encontradas para usuário com id: '" + User::idUsuario + "'"); }

  return query.value(permissao).toBool();
}

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
  // TODO: precisa adicionar 'GERENTE FINANCEIRO'

  SqlQuery query;
  query.prepare(
      "SELECT idUsuario, valorMinimoFrete FROM usuario WHERE user = :user AND senhaUsoUnico = :senhaUsoUnico AND tipo IN ('ADMINISTRADOR', 'ADMINISTRATIVO', 'DIRETOR', 'GERENTE DEPARTAMENTO', 'GERENTE LOJA')");
  query.bindValue(":user", user);
  query.bindValue(":senhaUsoUnico", password);

  if (not query.exec()) { throw RuntimeException("Erro ao consultar senha: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeError("Senha não confere!"); }

  valorMinimoFrete = query.value("valorMinimoFrete").toDouble();

  SqlQuery query2;

  if (not query2.exec("UPDATE usuario SET senhaUsoUnico = NULL, valorMinimoFrete = NULL WHERE user = '" + user + "'")) { throw RuntimeException("Erro ao apagar senha de uso único: " + query2.lastError().text()); }
}

QVariant User::fromLoja(const QString &parameter, const QString &idUsuario_) {
  SqlQuery queryLoja;
  queryLoja.prepare("SELECT " + parameter + " FROM loja LEFT JOIN usuario ON loja.idLoja = usuario.idLoja WHERE usuario.idUsuario = :idUsuario");
  queryLoja.bindValue(":idUsuario", idUsuario_);

  if (not queryLoja.exec()) { throw RuntimeException("Erro na query loja: " + queryLoja.lastError().text()); }

  if (not queryLoja.first()) { throw RuntimeException("Dados da loja/usuário não encontrados para o usuário: '" + idUsuario_ + "'"); }

  return queryLoja.value(0);
}
