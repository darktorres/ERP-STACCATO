#include "log.h"

#include "application.h"
#include "user.h"

#include <QDebug>

void Log::createLog(const QString &tipo, const QString &message) {
  if (not QSqlDatabase::database().isOpen()) { return; }

  SqlQuery query;
  query.prepare("INSERT INTO log (idUsuario, versao, tipo, message) VALUES (:idUsuario, :versao, :tipo, :message)");
  query.bindValue(":idUsuario", User::idUsuario);
  query.bindValue(":versao", qApp->applicationVersion());
  query.bindValue(":tipo", tipo);
  query.bindValue(":message", message);

  qDebug() << "save log: " << query.exec();
}
