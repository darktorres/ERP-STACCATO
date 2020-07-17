#pragma once

#include "sortfilterproxymodel.h"
#include "sqltablemodel.h"
#include "sqltreemodel.h"

class SearchDialogProxyModel final : public SortFilterProxyModel {

public:
  explicit SearchDialogProxyModel(SqlTableModel *model, QObject *parent);
  explicit SearchDialogProxyModel(SqlTreeModel *model, QObject *parent);
  ~SearchDialogProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant final;

private:
  const int estoqueColumn = -1;
  const int promocaoColumn = -1;
  const int descontinuadoColumn = -1;
  const int validadeColumn = -1;

  // QSortFilterProxyModel interface
protected:
  auto lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const -> bool final;
};
