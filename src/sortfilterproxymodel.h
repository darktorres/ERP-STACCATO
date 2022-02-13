#pragma once

#include "sqltreemodel.h"

#include <QSortFilterProxyModel>
#include <QSqlQueryModel>

class SortFilterProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

public:
  explicit SortFilterProxyModel(QSqlQueryModel *model, QObject *parent);
  explicit SortFilterProxyModel(SqlTreeModel *model, QObject *parent);

private:
  // attributes
  int const statusColumn;
  // methods
};
