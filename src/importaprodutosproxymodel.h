#pragma once

#include "sqlrelationaltablemodel.h"

#include <QIdentityProxyModel>

class ImportaProdutosProxyModel final : public QIdentityProxyModel {

public:
  explicit ImportaProdutosProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~ImportaProdutosProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int descontinuadoColumn;
  enum class Status { Novo = 1, Atualizado = 2, ForaPadrao = 3, Errado = 4 };
};
