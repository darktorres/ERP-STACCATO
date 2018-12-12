#ifndef ESTOQUEPROXYMODEL_H
#define ESTOQUEPROXYMODEL_H

#include <QIdentityProxyModel>

#include "sqlrelationaltablemodel.h"

// TODO: change this to inherit from QSortFilterProxyModel
class EstoqueProxyModel final : public QIdentityProxyModel {

public:
  explicit EstoqueProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~EstoqueProxyModel() final = default;
  auto data(const QModelIndex &proxyIndex, const int role) const -> QVariant final;

private:
  const int quantUpdIndex;
  enum class Status { Ok = 1, QuantDifere = 2, NaoEncontrado = 3, Consumo = 4, Devolucao = 5 };
};

#endif // ESTOQUEPROXYMODEL_H

//  auto *proxyFilter = new QSortFilterProxyModel(this);
//  proxyFilter->setDynamicSortFilter(true);
//  proxyFilter->setSourceModel(&modelEstoque);
//  ui->tableEstoque->setModel(proxyFilter);
