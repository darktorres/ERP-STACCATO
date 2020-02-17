#include "log.h"

#include "application.h"
#include "usersession.h"

#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

bool Log::createLog(const QString &message) {
  QSqlQuery query;
  query.prepare("INSERT INTO log (idUsuario, versao, message) VALUES (:idUsuario, :versao, :message)");
  query.bindValue(":idUsuario", UserSession::idUsuario());
  query.bindValue(":versao", qApp->applicationVersion());
  query.bindValue(":message", message);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro salvando log: " + query.lastError().text()); }

  return true;
}
