#include "sqlquery.h"

#include "application.h"

#include <QSqlError>
#include <QSqlRecord>

SqlQuery::SqlQuery() : QSqlQuery() {}

bool SqlQuery::exec() { return QSqlQuery::exec(); }

bool SqlQuery::exec(const QString &query) { return QSqlQuery::exec(query); }

const QVariant SqlQuery::value(const QString &name) {
  const int index = record().indexOf(name);

  if (not isActive()) { throw RuntimeException("Query not active"); }
  if (not isValid()) { throw RuntimeException("Query not valid"); }
  if (index == -1) { throw RuntimeException("Query index -1"); }

  const QVariant variant = QSqlQuery::value(index);

  if (not variant.isValid()) { throw RuntimeException("Variant invalid"); }

  return variant;
}
