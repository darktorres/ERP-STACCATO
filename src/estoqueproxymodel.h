#pragma once

#include "sortfilterproxymodel.h"

#include <QSqlQueryModel>

class EstoqueProxyModel final : public SortFilterProxyModel {

public:
  explicit EstoqueProxyModel(QSqlQueryModel *model, QObject *parent);
  ~EstoqueProxyModel() final = default;

private:
  enum class Status { Ok = 1, QuantDifere = 2, NaoEncontrado = 3, Consumo = 4, Devolucao = 5 };
  // attributes
  const int quantUpdColumn = -1;
  // methods
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;
};
