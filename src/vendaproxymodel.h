#pragma once

#include <QIdentityProxyModel>
#include <QSqlQueryModel>

class VendaProxyModel final : public QIdentityProxyModel {

public:
  explicit VendaProxyModel(QSqlQueryModel *model, QObject *parent);
  ~VendaProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  int const diasRestantesIndex;
  int const financeiroIndex;
  int const statusIndex;

  enum class FieldColors { Quente = 1, Morno = 2, Frio = 3 };
};
