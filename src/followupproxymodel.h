#ifndef FOLLOWUPPROXYMODEL_H
#define FOLLOWUPPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class FollowUpProxyModel final : public QIdentityProxyModel {

public:
  FollowUpProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~FollowUpProxyModel() = default;
  QVariant data(const QModelIndex &proxyIndex, int role) const final;

private:
  const int semaforo;

  enum class FieldColors { Quente = 1, Morno = 2, Frio = 3 };
};

#endif // FOLLOWUPPROXYMODEL_H
