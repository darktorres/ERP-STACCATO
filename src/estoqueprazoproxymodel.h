#pragma once

#include <QIdentityProxyModel>
#include <QSqlQueryModel>

class EstoquePrazoProxyModel final : public QIdentityProxyModel {
  Q_OBJECT

public:
  explicit EstoquePrazoProxyModel(QSqlQueryModel *model, QObject *parent);
  ~EstoquePrazoProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  int const prazoEntregaColumn;
};
