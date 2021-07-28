#pragma once

#include "sortfilterproxymodel.h"
#include "sqltablemodel.h"
#include "sqltreemodel.h"

#include <QSqlQueryModel>

class ProdutoProxyModel final : public SortFilterProxyModel {
  Q_OBJECT

public:
  explicit ProdutoProxyModel(QSqlQueryModel *model, QObject *parent);
  explicit ProdutoProxyModel(SqlTreeModel *model, QObject *parent);
  ~ProdutoProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant final;

  // QSortFilterProxyModel interface
protected:
  auto lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const -> bool final;

private:
  // attributes
  int const descontinuadoColumn;
  int const estoqueColumn;
  int const promocaoColumn;
  // methods
};
