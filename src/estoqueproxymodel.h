#pragma once

#include "sortfilterproxymodel.h"

#include <QSqlQueryModel>

class EstoqueProxyModel final : public SortFilterProxyModel {
  Q_OBJECT

public:
  enum class Status { Ok = 1, QuantDifere = 2, NaoEncontrado = 3, Consumo = 4, Devolucao = 5 };
  Q_ENUM(Status)

  explicit EstoqueProxyModel(QSqlQueryModel *model, QObject *parent);
  ~EstoqueProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  // attributes
  int const quantUpdColumn;
  // methods
};
