#pragma once

#include <QIdentityProxyModel>
#include <QSqlQueryModel>

class OrcamentoProxyModel final : public QIdentityProxyModel {
  Q_OBJECT

public:
  enum class FieldColors { Quente = 1, Morno = 2, Frio = 3 };
  Q_ENUM(FieldColors)

  explicit OrcamentoProxyModel(QSqlQueryModel *model, QObject *parent);
  ~OrcamentoProxyModel() final = default;

  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  int const diasRestantesIndex;
  int const followupIndex;
  int const semaforoIndex;
  int const statusIndex;
};
