#pragma once

#include <QIdentityProxyModel>
#include <QSqlQueryModel>

class FinanceiroProxyModel final : public QIdentityProxyModel {

public:
  explicit FinanceiroProxyModel(QSqlQueryModel *model, QObject *parent);
  ~FinanceiroProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant final;

private:
  const int statusFinanceiro;
  const int prazoEntrega;
  const int novoPrazoEntrega;
};
