#ifndef ESTOQUEPROXYMODEL_H
#define ESTOQUEPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class EstoqueProxyModel final : public QIdentityProxyModel {

public:
  explicit EstoqueProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~EstoqueProxyModel() = default;
  QVariant data(const QModelIndex &proxyIndex, const int role) const final;

private:
  const int quantUpdIndex;
  enum class Status { Ok = 1, QuantDifere, NaoEncontrado, Consumo, Devolucao };
};

#endif // ESTOQUEPROXYMODEL_H
