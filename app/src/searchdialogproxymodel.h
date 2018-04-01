#ifndef SEARCHDIALOGPROXY_H
#define SEARCHDIALOGPROXY_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class SearchDialogProxyModel final : public QIdentityProxyModel {

public:
  SearchDialogProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~SearchDialogProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant final;

private:
  const int estoque;
  const int promocao;
  const int descontinuado;
  const int validade;
};

#endif // SEARCHDIALOGPROXY_H
