#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "log.h"
#include "usersession.h"

// Log::Log() {}

bool Log::createLog(const QString &message) {
  QSqlQuery query;
  query.prepare("INSERT INTO log (idUsuario, message) VALUES (:idUsuario, :message)");
  query.bindValue(":idUsuario", UserSession::idUsuario());
  query.bindValue(":message", message);

  if (not query.exec()) {
    QMessageBox::critical(nullptr, "Erro!", "Erro salvando log: " + query.lastError().text());
    return false;
  }

  return true;
}
