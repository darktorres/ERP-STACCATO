#pragma once

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class FinanceiroProxyModel final : public QIdentityProxyModel {

public:
  explicit FinanceiroProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~FinanceiroProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant final;

private:
  const int statusFinanceiro;
  const int prazoEntrega;
  const int novoPrazoEntrega;
};
