#pragma once

#include "sortfilterproxymodel.h"

#include <QSqlQueryModel>

class NFeProxyModel final : public SortFilterProxyModel {
  Q_OBJECT

public:
  explicit NFeProxyModel(QSqlQueryModel *model, QObject *parent);
  ~NFeProxyModel() = default;

  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  // attributes
  int const dataColumn;
  int const statusColumn;
  // methods
};
