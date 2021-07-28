#pragma once

#include <QIdentityProxyModel>
#include <QSqlQueryModel>

class VendaProxyModel final : public QIdentityProxyModel {
  Q_OBJECT

public:
  enum class FieldColors { Quente = 1, Morno = 2, Frio = 3 };
  Q_ENUM(FieldColors)

  explicit VendaProxyModel(QSqlQueryModel *model, QObject *parent);
  ~VendaProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  int const diasRestantesIndex;
  int const financeiroIndex;
  int const statusIndex;
};
