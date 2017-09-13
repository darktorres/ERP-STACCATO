#include <QBrush>
#include <ciso646>

#include "importaprodutosproxy.h"

ImportaProdutosProxy::ImportaProdutosProxy(SqlTableModel *model, QObject *parent) : QIdentityProxyModel(parent), descontinuado(model->fieldIndex("descontinuado")) { setSourceModel(model); }

QVariant ImportaProdutosProxy::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole) {

    // verifica se está descontinuado
    const int descontinuado = QIdentityProxyModel::data(index(proxyIndex.row(), this->descontinuado), Qt::DisplayRole).toInt();

    if (descontinuado == true) return QBrush(Qt::cyan);

    // verifica cada campo
    for (int column = 0, columns = columnCount(); column < columns; ++column) {
      if (proxyIndex.column() == column) {
        const int value = QIdentityProxyModel::data(index(proxyIndex.row(), column + 1), Qt::DisplayRole).toInt();

        // TODO: create a enum
        if (value == 0) return QBrush(Qt::white);
        if (value == 1) return QBrush(Qt::green);
        if (value == 2) return QBrush(Qt::yellow);
        if (value == 3) return QBrush(Qt::gray);
        if (value == 4) return QBrush(Qt::red);
      }
    }
  }

  if (role == Qt::ForegroundRole) return QBrush(Qt::black);

  return QIdentityProxyModel::data(proxyIndex, role);
}
