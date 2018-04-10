#ifndef HORIZONTALPROXYMODEL_H
#define HORIZONTALPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class HorizontalProxyModel final : public QIdentityProxyModel {

public:
  explicit HorizontalProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~HorizontalProxyModel() override = default;
  auto columnCount(const QModelIndex &parent = QModelIndex()) const -> int final;
  auto headerData(int section, Qt::Orientation orientation, int role) const -> QVariant final;
  auto index(int row, int column, const QModelIndex &parent = QModelIndex()) const -> QModelIndex final;
  auto mapFromSource(const QModelIndex &sourceIndex = QModelIndex()) const -> QModelIndex final;
  auto mapToSource(const QModelIndex &proxyIndex = QModelIndex()) const -> QModelIndex final;
  auto parent(const QModelIndex &child = QModelIndex()) const -> QModelIndex final;
  auto rowCount(const QModelIndex &parent = QModelIndex()) const -> int final;
};

#endif // HORIZONTALPROXYMODEL_H
