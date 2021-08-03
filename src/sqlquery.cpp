#include "sqlquery.h"

#include "application.h"

#include <QSqlError>
#include <QSqlRecord>

QVariant SqlQuery::value(const QString &name) const {
  const int index = record().indexOf(name);

  if (not isActive()) { throw RuntimeException("Query not active"); }
  if (not isValid()) { throw RuntimeException("Query not valid"); }
  if (index == -1) { throw RuntimeException("Query index -1"); }

  QVariant variant = QSqlQuery::value(index);

  if (not variant.isValid()) { throw RuntimeException("Variant invalid"); }

  return variant;
}

QVariant SqlQuery::value(const int i) const {
  if (not isActive()) { throw RuntimeException("Query not active"); }
  if (not isValid()) { throw RuntimeException("Query not valid"); }

  QVariant variant = QSqlQuery::value(i);

  if (not variant.isValid()) { throw RuntimeException("Variant invalid"); }

  return variant;
}
