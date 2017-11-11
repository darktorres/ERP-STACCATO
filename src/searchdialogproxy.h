#ifndef SEARCHDIALOGPROXY_H
#define SEARCHDIALOGPROXY_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

// REFAC: rename this to SearchDialogProxyModel (and others proxy models)
class SearchDialogProxy final : public QIdentityProxyModel {

public:
  SearchDialogProxy(SqlRelationalTableModel *model, QObject *parent = 0);
  ~SearchDialogProxy() = default;
  QVariant data(const QModelIndex &proxyIndex, int role) const final;

private:
  const int estoque;
  const int promocao;
  const int descontinuado;
  const int validade;
};

#endif // SEARCHDIALOGPROXY_H
