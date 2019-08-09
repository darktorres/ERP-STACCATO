#pragma once

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class OrcamentoProxyModel final : public QIdentityProxyModel {

public:
  explicit OrcamentoProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~OrcamentoProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int diasRestantesIndex;
  const int statusIndex;
  const int followupIndex;
  const int semaforoIndex;

  enum class FieldColors { Quente = 1, Morno = 2, Frio = 3 };
};
