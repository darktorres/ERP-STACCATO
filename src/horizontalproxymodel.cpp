#include "horizontalproxymodel.h"

HorizontalProxyModel::HorizontalProxyModel(SqlRelationalTableModel *model, QObject *parent) : QIdentityProxyModel(parent) { setSourceModel(model); }

QModelIndex HorizontalProxyModel::mapToSource(const QModelIndex &proxyIndex) const { return sourceModel()->index(proxyIndex.column(), proxyIndex.row()); }

QModelIndex HorizontalProxyModel::mapFromSource(const QModelIndex &sourceIndex) const { return index(sourceIndex.column(), sourceIndex.row()); }

QModelIndex HorizontalProxyModel::index(int row, int column, const QModelIndex &) const { return createIndex(row, column); }

QModelIndex HorizontalProxyModel::parent(const QModelIndex &) const { return QModelIndex(); }

int HorizontalProxyModel::rowCount(const QModelIndex &) const { return sourceModel()->columnCount(); }

int HorizontalProxyModel::columnCount(const QModelIndex &) const { return sourceModel()->rowCount(); }

QVariant HorizontalProxyModel::headerData(int section, Qt::Orientation orientation, int role) const {
  Qt::Orientation new_orientation = (orientation == Qt::Horizontal) ? Qt::Vertical : Qt::Horizontal;
  return sourceModel()->headerData(section, new_orientation, role);
}
