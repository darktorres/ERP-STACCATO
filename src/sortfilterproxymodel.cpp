#include "sortfilterproxymodel.h"

#include <QDebug>
#include <QFont>
#include <QSqlRecord>

SortFilterProxyModel::SortFilterProxyModel(QSqlQueryModel *model, QObject *parent) : QSortFilterProxyModel(parent), statusColumn(model->record().indexOf("status")) {
  setSourceModel(model);
  setDynamicSortFilter(false);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

SortFilterProxyModel::SortFilterProxyModel(SqlTreeModel *model, QObject *parent) : QSortFilterProxyModel(parent), statusColumn(model->fieldIndex("status")) {
  setSourceModel(model);
  setDynamicSortFilter(false);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

QVariant SortFilterProxyModel::data(const QModelIndex &index, int role) const {
  if (statusColumn != -1 and role == Qt::FontRole) {
    const QString status = index.siblingAtColumn(statusColumn).data().toString();

    if (status == "CANCELADA" or status == "CANCELADO" or status == "SUBSTITUIDO") {
      QFont font;
      font.setStrikeOut(true);
      return font;
    }
  }

  return QSortFilterProxyModel::data(index, role);
}
