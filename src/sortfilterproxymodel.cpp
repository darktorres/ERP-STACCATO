#include "sortfilterproxymodel.h"

SortFilterProxyModel::SortFilterProxyModel(QAbstractItemModel *model, QObject *parent) : QSortFilterProxyModel(parent) {
  setSourceModel(model);
  setDynamicSortFilter(false);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}
