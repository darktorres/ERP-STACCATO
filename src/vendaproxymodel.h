#ifndef VENDAPROXYMODEL_H
#define VENDAPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class VendaProxyModel final : public QIdentityProxyModel {

public:
  explicit VendaProxyModel(SqlRelationalTableModel *model, QObject *parent);
  ~VendaProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int diasRestantesIndex;
  const int statusIndex;
  const int financeiroIndex;

  enum class FieldColors { Quente = 1, Morno = 2, Frio = 3 };
};

#endif // VENDAPROXYMODEL_H
