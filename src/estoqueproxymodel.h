#ifndef ESTOQUEPROXYMODEL_H
#define ESTOQUEPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class EstoqueProxyModel final : public QIdentityProxyModel {

public:
  explicit EstoqueProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~EstoqueProxyModel() = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int quantUpdIndex;
  enum class Status { Ok = 1, QuantDifere = 2, NaoEncontrado = 3, Consumo = 4, Devolucao = 5 };
};

#endif // ESTOQUEPROXYMODEL_H
