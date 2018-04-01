#ifndef IMPORTAPRODUTOSPROXY_H
#define IMPORTAPRODUTOSPROXY_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

class ImportaProdutosProxyModel final : public QIdentityProxyModel {

public:
  ImportaProdutosProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~ImportaProdutosProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int descontinuado;
  enum class Status { Novo = 1, Atualizado = 2, ForaPadrao = 3, Errado = 4 };
};

#endif // IMPORTAPRODUTOSPROXY_H