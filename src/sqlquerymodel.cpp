#include "sqlquerymodel.h"

#include "application.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>

SqlQueryModel::SqlQueryModel(QObject *parent) : QSqlQueryModel(parent) {}

SqlQueryModel::SqlQueryModel() : SqlQueryModel(nullptr) {}

QVariant SqlQueryModel::data(const int row, const int column) const {
  if (row == -1 or column == -1) {
    qApp->enqueueException("Erro: linha/coluna -1 SqlQueryModel");
    return QVariant();
  }

  if (proxyModel) { return proxyModel->data(proxyModel->index(row, column)); }

  return QSqlQueryModel::data(QSqlQueryModel::index(row, column));
}

QVariant SqlQueryModel::data(const QModelIndex &index, const QString &column) const { return data(index.row(), column); }

QVariant SqlQueryModel::data(const int row, const QString &column) const { return data(row, fieldIndex(column)); }

bool SqlQueryModel::setHeaderData(const QString &column, const QVariant &value) {
  const int field = fieldIndex(column);

  if (field == -1) { return qApp->enqueueException(false, "Coluna '" + column + "' não encontrada na tabela!"); }

  return QSqlQueryModel::setHeaderData(field, Qt::Horizontal, value);
}

bool SqlQueryModel::setQuery(const QString &query, const QSqlDatabase &db) {
  // TODO: redo places that use this function

  m_query = query;

  QSqlQueryModel::setQuery(query, db);

  if (lastError().isValid()) { return qApp->enqueueException(false, "Erro lendo dados: " + lastError().text()); }

  return true;
}

bool SqlQueryModel::setQuery2(const QString &query, const QSqlDatabase &db) {
  QSqlQueryModel::setQuery(query, db);

  if (lastError().isValid()) { return qApp->enqueueException(false, "Erro lendo dados: " + lastError().text()); }

  return true;
}

int SqlQueryModel::fieldIndex(const QString &fieldName, const bool silent) const {
  const int field = record().indexOf(fieldName);

  if (field == -1 and not silent) { qApp->enqueueException(fieldName + " não encontrado na tabela!"); }

  return field;
}

void SqlQueryModel::sort(int column, Qt::SortOrder order) { setQuery2(m_query + " ORDER BY " + record().fieldName(column) + (order == Qt::AscendingOrder ? " ASC" : " DESC")); }
