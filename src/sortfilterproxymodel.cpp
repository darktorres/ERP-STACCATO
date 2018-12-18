#include "sortfilterproxymodel.h"
#include "application.h"

#include <QSqlRecord>

SortFilterProxyModel::SortFilterProxyModel(QSqlTableModel *model, QObject *parent) : QSortFilterProxyModel(parent), tableModel(model) { setSourceModel(model); }

SortFilterProxyModel::SortFilterProxyModel(QSqlQueryModel *model, QObject *parent) : QSortFilterProxyModel(parent), queryModel(model) { setSourceModel(model); }

QVariant SortFilterProxyModel::data(const int row, const int column, int role) const { return QSortFilterProxyModel::data(index(row, column), role); }

QVariant SortFilterProxyModel::data(const int row, const QString &column, int role) const {
  if (tableModel) {
    if (tableModel->fieldIndex(column) == -1) {
      qApp->enqueueError("Chave '" + column + "' não encontrada na tabela " + tableModel->tableName());
      return QVariant();
    }

    return QSortFilterProxyModel::data(index(row, tableModel->fieldIndex(column)), role);
  }

  if (queryModel) {
    const int indexOf = queryModel->record().indexOf(column);

    if (indexOf == -1) {
      qApp->enqueueError("Coluna '" + column + "' não encontada na tabela!");
      return QVariant();
    }

    return QSortFilterProxyModel::data(index(row, queryModel->record().indexOf(column)), role);
  }

  return QVariant();
}
