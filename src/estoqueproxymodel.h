#pragma once

#include "sortfilterproxymodel.h"
#include "sqlrelationaltablemodel.h"

class EstoqueProxyModel final : public SortFilterProxyModel {

public:
  explicit EstoqueProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~EstoqueProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int quantUpdIndex;
  enum class Status { Ok = 1, QuantDifere = 2, NaoEncontrado = 3, Consumo = 4, Devolucao = 5 };
};
