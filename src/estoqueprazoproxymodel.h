#pragma once

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class EstoquePrazoProxyModel final : public QIdentityProxyModel {

public:
  explicit EstoquePrazoProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~EstoquePrazoProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int prazoEntregaColumn;
};
