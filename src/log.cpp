#include "log.h"

#include "application.h"
#include "usersession.h"

#include <QSqlError>
#include <QSqlQuery>

bool Log::createLog(const QString &tipo, const QString &message, const bool silent) {
  QSqlQuery query;
  query.prepare("INSERT INTO log (idUsuario, versao, tipo, message) VALUES (:idUsuario, :versao, :tipo, :message)");
  query.bindValue(":idUsuario", UserSession::idUsuario());
  query.bindValue(":versao", qApp->applicationVersion());
  query.bindValue(":tipo", tipo);
  query.bindValue(":message", message);

  // TODO: testar se isso nao entra em loop ao chamar enqueueException
  if (not query.exec() and not silent) { return qApp->enqueueException(false, "Erro salvando log: " + query.lastError().text()); }

  return true;
}
