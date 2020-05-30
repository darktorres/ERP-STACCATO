#pragma once

#include "sqltablemodel.h"

#include <QIdentityProxyModel>

class FinanceiroProxyModel final : public QIdentityProxyModel {

public:
  explicit FinanceiroProxyModel(SqlTableModel *model, QObject *parent);
  ~FinanceiroProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant final;

private:
  const int statusFinanceiro;
  const int prazoEntrega;
  const int novoPrazoEntrega;
};
