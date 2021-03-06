#pragma once

#include <QIdentityProxyModel>
#include <QSqlQueryModel>

class ImportaProdutosProxyModel final : public QIdentityProxyModel {

public:
  explicit ImportaProdutosProxyModel(QSqlQueryModel *model, QObject *parent);
  ~ImportaProdutosProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  int const descontinuadoColumn;
  enum class Status { Novo = 1, Atualizado = 2, ForaPadrao = 3, Errado = 4 };
  Q_ENUM(Status)
};
