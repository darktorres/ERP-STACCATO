#pragma once

#include <QIdentityProxyModel>
#include <QSqlQueryModel>

class VendaProxyModel final : public QIdentityProxyModel {
  Q_OBJECT

public:
  explicit VendaProxyModel(QSqlQueryModel *model, QObject *parent);
  ~VendaProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  int const diasRestantesIndex;
  int const financeiroIndex;
  int const statusIndex;
};
