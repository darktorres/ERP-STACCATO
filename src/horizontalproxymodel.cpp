#include "horizontalproxymodel.h"

HorizontalProxyModel::HorizontalProxyModel(SqlRelationalTableModel *model, QObject *parent) : QIdentityProxyModel(parent) { setSourceModel(model); }

QModelIndex HorizontalProxyModel::mapToSource(const QModelIndex &proxyIndex) const { return sourceModel() ? sourceModel()->index(proxyIndex.column(), proxyIndex.row()) : QModelIndex(); }

QModelIndex HorizontalProxyModel::mapFromSource(const QModelIndex &sourceIndex) const { return index(sourceIndex.column(), sourceIndex.row()); }

QModelIndex HorizontalProxyModel::index(int row, int column, const QModelIndex &) const { return createIndex(row, column, nullptr); }

QModelIndex HorizontalProxyModel::parent(const QModelIndex &) const { return QModelIndex(); }

int HorizontalProxyModel::rowCount(const QModelIndex &) const { return sourceModel() ? sourceModel()->columnCount() : 0; }

int HorizontalProxyModel::columnCount(const QModelIndex &) const { return sourceModel() ? sourceModel()->rowCount() : 0; }

QVariant HorizontalProxyModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (not sourceModel()) { return QVariant(); }

  Qt::Orientation new_orientation = orientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal;
  return sourceModel()->headerData(section, new_orientation, role);
}
