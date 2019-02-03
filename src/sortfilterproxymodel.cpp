#include "sortfilterproxymodel.h"

SortFilterProxyModel::SortFilterProxyModel(QAbstractItemModel *model, QObject *parent) : QSortFilterProxyModel(parent) { setSourceModel(model); }

QVariant SortFilterProxyModel::data(const int row, const int column, int role) const { return QSortFilterProxyModel::data(index(row, column), role); }
