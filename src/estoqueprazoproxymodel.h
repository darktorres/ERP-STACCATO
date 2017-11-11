#ifndef ESTOQUEPRAZOPROXYMODEL_H
#define ESTOQUEPRAZOPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class EstoquePrazoProxyModel final : public QIdentityProxyModel {

public:
  explicit EstoquePrazoProxyModel(SqlRelationalTableModel *model, QObject *parent);
  ~EstoquePrazoProxyModel() = default;
  QVariant data(const QModelIndex &proxyIndex, const int role) const final;

private:
  const int dias;
};

#endif // ESTOQUEPRAZOPROXYMODEL_H
