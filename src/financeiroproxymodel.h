#ifndef FINANCEIROPROXYMODEL_H
#define FINANCEIROPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class FinanceiroProxyModel : public QIdentityProxyModel {

public:
  FinanceiroProxyModel(SqlRelationalTableModel *model, QObject *parent = 0);
  ~FinanceiroProxyModel() = default;
  QVariant data(const QModelIndex &proxyIndex, int role) const;

private:
  const int statusFinanceiro;
  const int prazoEntrega;
  const int novoPrazoEntrega;
};

#endif // FINANCEIROPROXYMODEL_H
