#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "log.h"
#include "usersession.h"

bool Log::createLog(const QString &message) {
  QSqlQuery query;
  query.prepare("INSERT INTO log (idUsuario, message) VALUES (:idUsuario, :message)");
  query.bindValue(":idUsuario", UserSession::idUsuario());
  query.bindValue(":message", message);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro salvando log: " + query.lastError().text()); }

  return true;
}
