#ifndef ORCAMENTOPROXYMODEL_H
#define ORCAMENTOPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class OrcamentoProxyModel final : public QIdentityProxyModel {

public:
  explicit OrcamentoProxyModel(SqlRelationalTableModel *model, QObject *parent);
  ~OrcamentoProxyModel() = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int diasRestantesIndex;
  const int statusIndex;
  const int followupIndex;
  const int semaforoIndex;

  enum class FieldColors { Quente = 1, Morno = 2, Frio = 3 };
};

#endif // ORCAMENTOPROXYMODEL_H
