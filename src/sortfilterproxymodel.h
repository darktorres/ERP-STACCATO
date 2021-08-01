#pragma once

#include "sqltreemodel.h"

#include <QSortFilterProxyModel>
#include <QSqlQueryModel>

class SortFilterProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

public:
  explicit SortFilterProxyModel(QSqlQueryModel *model, QObject *parent);
  explicit SortFilterProxyModel(SqlTreeModel *model, QObject *parent);

  auto data(const QModelIndex &index, int role) const -> QVariant override;

private:
  // attributes
  int const statusColumn;
  int const quantColumn;
  int const unidadeColumn;
  // methods
};
