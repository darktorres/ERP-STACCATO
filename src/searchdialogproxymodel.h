#pragma once

#include "sortfilterproxymodel.h"
#include "sqltablemodel.h"
#include "sqltreemodel.h"

#include <QSqlQueryModel>

class SearchDialogProxyModel final : public SortFilterProxyModel {

public:
  explicit SearchDialogProxyModel(QSqlQueryModel *model, QObject *parent);
  explicit SearchDialogProxyModel(SqlTreeModel *model, QObject *parent);
  ~SearchDialogProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant final;

private:
  int const descontinuadoColumn = -1;
  int const estoqueColumn = -1;
  int const promocaoColumn = -1;
  int const validadeColumn = -1;

  // QSortFilterProxyModel interface
protected:
  auto lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const -> bool final;
};
