#pragma once

#include "sortfilterproxymodel.h"

#include <QSqlQueryModel>

class ImportaProdutosProxyModel final : public SortFilterProxyModel {
  Q_OBJECT

public:
  enum class Status { Novo = 1, Atualizado = 2, ForaPadrao = 3, Errado = 4 };
  Q_ENUM(Status)

  explicit ImportaProdutosProxyModel(QSqlQueryModel *model, QObject *parent);
  ~ImportaProdutosProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  // attributes
  int const descontinuadoColumn;
  // methods
};
