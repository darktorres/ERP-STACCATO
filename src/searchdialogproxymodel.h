#pragma once

#include "sortfilterproxymodel.h"
#include "sqltablemodel.h"
#include "sqltreemodel.h"

#include <QSqlQueryModel>

class SearchDialogProxyModel final : public SortFilterProxyModel {
  Q_OBJECT

public:
  explicit SearchDialogProxyModel(QSqlQueryModel *model, QObject *parent);
  explicit SearchDialogProxyModel(SqlTreeModel *model, QObject *parent);
  ~SearchDialogProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant final;

  // QSortFilterProxyModel interface
protected:
  auto lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const -> bool final;

private:
  // attributes
  int const descontinuadoColumn;
  int const estoqueColumn;
  int const promocaoColumn;
  int const validadeColumn;
  // methods
};
