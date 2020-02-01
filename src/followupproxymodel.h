#pragma once

#include "sqlrelationaltablemodel.h"

#include <QIdentityProxyModel>

class FollowUpProxyModel final : public QIdentityProxyModel {

public:
  explicit FollowUpProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~FollowUpProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant final;

private:
  const int semaforoColumn;
  enum class FieldColors { Quente = 1, Morno = 2, Frio = 3 };
};
