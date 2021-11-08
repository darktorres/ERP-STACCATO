#include "sqlquery.h"

#include "application.h"

#include <QSqlError>
#include <QSqlRecord>

QVariant SqlQuery::value(const QString &name) const {
  const int index = record().indexOf(name);

  if (not isActive()) { throw RuntimeException("Query não está ativa!"); }
  if (not isValid()) { throw RuntimeException("Query não é válida!"); }
  if (index == -1) { throw RuntimeException("Coluna " + name + " não encontrada na query!"); }

  QVariant variant = QSqlQuery::value(index);

  if (not variant.isValid()) { throw RuntimeException("Valor da query não é válido!"); }

  return variant;
}

QVariant SqlQuery::value(const int i) const {
  if (not isActive()) { throw RuntimeException("Query não está ativa!"); }
  if (not isValid()) { throw RuntimeException("Query não é válida!"); }

  QVariant variant = QSqlQuery::value(i);

  if (not variant.isValid()) { throw RuntimeException("Valor da query não é válido!"); }

  return variant;
}
