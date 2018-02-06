#ifndef FINANCEIROPROXYMODEL_H
#define FINANCEIROPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class FinanceiroProxyModel final : public QIdentityProxyModel {

public:
  FinanceiroProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~FinanceiroProxyModel() = default;
  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant;

private:
  const int statusFinanceiro;
  const int prazoEntrega;
  const int novoPrazoEntrega;
};

#endif // FINANCEIROPROXYMODEL_H
