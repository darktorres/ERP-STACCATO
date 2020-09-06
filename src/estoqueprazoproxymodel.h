#pragma once

#include <QIdentityProxyModel>
#include <QSqlQueryModel>

class EstoquePrazoProxyModel final : public QIdentityProxyModel {

public:
  explicit EstoquePrazoProxyModel(QSqlQueryModel *model, QObject *parent);
  ~EstoquePrazoProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int prazoEntregaColumn;
};
