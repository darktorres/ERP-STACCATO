#include <QSqlError>
#include <QSqlRecord>

#include "application.h"
#include "sqlquerymodel.h"

SqlQueryModel::SqlQueryModel(QObject *parent) : QSqlQueryModel(parent) {}

QVariant SqlQueryModel::data(const QModelIndex &index, int role) const { return QSqlQueryModel::data(index, role); }

QVariant SqlQueryModel::data(const QModelIndex &index, const QString &column) const { return data(index.row(), column); }

QVariant SqlQueryModel::data(const int row, const QString &column) const {
  const int index = QSqlQueryModel::record().indexOf(column);

  if (index == -1) {
    qApp->enqueueError("Coluna '" + column + "' não encontrada na tabela!");
    return QVariant();
  }

  return QSqlQueryModel::data(QSqlQueryModel::index(row, index));
}

bool SqlQueryModel::setHeaderData(const QString &column, const QVariant &value) {
  const int index = QSqlQueryModel::record().indexOf(column);

  if (index == -1) { return qApp->enqueueError(false, "Coluna '" + column + "' não encontrada na tabela!"); }

  return QSqlQueryModel::setHeaderData(index, Qt::Horizontal, value);
}

bool SqlQueryModel::setQuery(const QString &query, const QSqlDatabase &db) {
  // TODO: redo places that use this function

  QSqlQueryModel::setQuery(query, db);

  if (lastError().isValid()) { return qApp->enqueueError(false, "Erro lendo dados: " + lastError().text()); }

  return true;
}
