#ifndef IMPORTAPRODUTOSPROXY_H
#define IMPORTAPRODUTOSPROXY_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class ImportaProdutosProxy final : public QIdentityProxyModel {

public:
  ImportaProdutosProxy(SqlRelationalTableModel *model, QObject *parent = 0);
  ~ImportaProdutosProxy() = default;
  QVariant data(const QModelIndex &proxyIndex, const int role) const final;

private:
  const int descontinuado;
  enum class Status { Novo = 1, Atualizado, ForaPadrao, Errado };
};

#endif // IMPORTAPRODUTOSPROXY_H
