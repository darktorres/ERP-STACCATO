#pragma once

#include "sqltablemodel.h"

#include <QIdentityProxyModel>

class EstoquePrazoProxyModel final : public QIdentityProxyModel {

public:
  explicit EstoquePrazoProxyModel(SqlTableModel *model, QObject *parent);
  ~EstoquePrazoProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int prazoEntregaColumn;
};
