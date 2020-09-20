#include "log.h"

#include "application.h"
#include "usersession.h"

#include <QSqlError>
#include <QSqlQuery>

void Log::createLog(const QString &tipo, const QString &message) {
  if (not QSqlDatabase::database().isOpen()) { return; }

  QSqlQuery query;
  query.prepare("INSERT INTO log (idUsuario, versao, tipo, message) VALUES (:idUsuario, :versao, :tipo, :message)");
  query.bindValue(":idUsuario", UserSession::idUsuario);
  query.bindValue(":versao", qApp->applicationVersion());
  query.bindValue(":tipo", tipo);
  query.bindValue(":message", message);

  query.exec();
}
