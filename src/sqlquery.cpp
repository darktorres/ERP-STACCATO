#include "sqlquery.h"

#include "application.h"

#include <QSqlError>
#include <QSqlRecord>

// void SqlQuery::bindValue(const QString &placeholder, const QVariant &val, const bool adjustValue, QSql::ParamType type) {
//  QVariant adjustedValue = val;

// TODO: apenas senha e XML não podem ser alterados
// XML pode ser detectado verificando se começa com < ou marcador BOM
// verificar aqui se a string é XML para evitar alterar setData em vários locais, podendo esquecer de alterar e o XML ser corrompido

//  if (adjustValue and adjustedValue.userType() == QMetaType::Double) { adjustedValue.setValue(qApp->roundDouble(adjustedValue.toDouble())); }
//  if (adjustValue and adjustedValue.userType() == QMetaType::QString) { adjustedValue.setValue(adjustedValue.toString().toUpper().trimmed()); }

//  QSqlQuery::bindValue(placeholder, adjustedValue, type);
//}

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

// TODO: reimplementar bindValue e avisar se o campo não for encontrado na query preparada
