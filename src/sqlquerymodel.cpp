#include "sqlquerymodel.h"

#include "application.h"

#include <QDebug>
#include <QFont>
#include <QSqlError>
#include <QSqlRecord>

SqlQueryModel::SqlQueryModel(QObject *parent) : QSqlQueryModel(parent) {}

SqlQueryModel::SqlQueryModel() : SqlQueryModel(nullptr) {}

QVariant SqlQueryModel::data(const int row, const int column) const {
  if (row == -1 or column == -1) { throw RuntimeException("Erro: linha/coluna -1 SqlQueryModel"); }

  if (proxyModel) { return proxyModel->data(proxyModel->index(row, column)); }

  return QSqlQueryModel::data(QSqlQueryModel::index(row, column));
}

QVariant SqlQueryModel::data(const QModelIndex &index, const QString &column) const { return data(index.row(), column); }

QVariant SqlQueryModel::data(const int row, const QString &column) const { return data(row, fieldIndex(column)); }

bool SqlQueryModel::setHeaderData(const QString &column, const QVariant &value) {
  const int field = fieldIndex(column);

  if (field == -1) { throw RuntimeException("Coluna '" + column + "' não encontrada na tabela!"); }

  return QSqlQueryModel::setHeaderData(field, Qt::Horizontal, value);
}

void SqlQueryModel::select() {
  QSqlQueryModel::setQuery(last_query);

  if (lastError().isValid()) { throw RuntimeException("Erro lendo dados: " + lastError().text()); }
}

void SqlQueryModel::select(const QString &query) {
  last_query = query;
  QSqlQueryModel::setQuery(query);

  if (lastError().isValid()) { throw RuntimeException("Erro lendo dados: " + lastError().text()); }

  statusColumn = fieldIndex("status", true);
}

void SqlQueryModel::setQuery(const QString &query, const QSqlDatabase &db) {
  Q_UNUSED(db)

  base_query = query;
  last_query = query;
}

int SqlQueryModel::fieldIndex(const QString &fieldName, const bool silent) const {
  const int field = record().indexOf(fieldName);

  if (field == -1 and not silent) { throw RuntimeException("\"" + fieldName + "\" não encontrado na tabela!"); }

  return field;
}

void SqlQueryModel::sort(const int column, const Qt::SortOrder order) { select(base_query + " ORDER BY `" + record().fieldName(column) + (order == Qt::AscendingOrder ? "` ASC" : "` DESC")); }

void SqlQueryModel::sort(const QString &column, const Qt::SortOrder order) { select(base_query + " ORDER BY " + column + (order == Qt::AscendingOrder ? " ASC" : " DESC")); }

QVariant SqlQueryModel::data(const QModelIndex &index, int role) const {
  if (statusColumn != -1 and role == Qt::FontRole) {
    const QString status = index.siblingAtColumn(statusColumn).data().toString();

    if (status == "CANCELADA" or status == "CANCELADO" or status == "SUBSTITUIDO") {
      QFont font;
      font.setStrikeOut(true);
      return font;
    }
  }

  return QSqlQueryModel::data(index, role);
}
