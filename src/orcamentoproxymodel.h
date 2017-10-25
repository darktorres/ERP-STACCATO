#ifndef ORCAMENTOPROXYMODEL_H
#define ORCAMENTOPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqltablemodel.h"

class OrcamentoProxyModel : public QIdentityProxyModel {

public:
  explicit OrcamentoProxyModel(SqlTableModel *model, QObject *parent);
  ~OrcamentoProxyModel() = default;
  QVariant data(const QModelIndex &proxyIndex, const int role) const override;

private:
  const int diasIndex;
  const int statusIndex;
  const int followupIndex;
  const int semaforoIndex;

  enum class FieldColors { Quente = 1, Morno = 2, Frio = 3 };
};

#endif // ORCAMENTOPROXYMODEL_H
