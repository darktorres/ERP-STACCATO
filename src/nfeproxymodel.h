#pragma once

#include "sortfilterproxymodel.h"

#include <QSqlQueryModel>

class NFeProxyModel final : public SortFilterProxyModel {

public:
  explicit NFeProxyModel(QSqlQueryModel *model, QObject *parent);
  ~NFeProxyModel() = default;

private:
  // attributes
  int const dataColumn = -1;
  int const statusColumn = -1;
  // methods
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;
};
