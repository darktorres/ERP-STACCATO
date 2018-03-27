#include <QApplication>
#include <QBrush>
#include <QStyle>

#include "importaprodutosproxymodel.h"

ImportaProdutosProxyModel::ImportaProdutosProxyModel(SqlRelationalTableModel *model, QObject *parent) : QIdentityProxyModel(parent), descontinuado(model->fieldIndex("descontinuado")) { setSourceModel(model); }

QVariant ImportaProdutosProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole) {
    // verifica se está descontinuado
    const bool descontinuado = QIdentityProxyModel::data(index(proxyIndex.row(), this->descontinuado), Qt::DisplayRole).toBool();

    if (descontinuado == true) return QBrush(Qt::cyan);

    // verifica cada campo
    for (int column = 0, columns = columnCount(); column < columns; ++column) {
      if (proxyIndex.column() == column) {
        const Status value = static_cast<Status>(QIdentityProxyModel::data(index(proxyIndex.row(), column + 1), Qt::DisplayRole).toInt());

        if (value == Status::Novo) return QBrush(Qt::green);
        if (value == Status::Atualizado) return QBrush(Qt::yellow);
        if (value == Status::ForaPadrao) return QBrush(Qt::gray);
        if (value == Status::Errado) return QBrush(Qt::red);
      }
    }
  }

  if (role == Qt::ForegroundRole) {

    // those paint the text as black if the background is colored

    const bool descontinuado = QIdentityProxyModel::data(index(proxyIndex.row(), this->descontinuado), Qt::DisplayRole).toBool();

    if (descontinuado == true) return QBrush(Qt::black);

    for (int column = 0, columns = columnCount(); column < columns; ++column) {
      if (proxyIndex.column() == column) {
        const Status value = static_cast<Status>(QIdentityProxyModel::data(index(proxyIndex.row(), column + 1), Qt::DisplayRole).toInt());

        if (value == Status::Novo) return QBrush(Qt::black);
        if (value == Status::Atualizado) return QBrush(Qt::black);
        if (value == Status::ForaPadrao) return QBrush(Qt::black);
        if (value == Status::Errado) return QBrush(Qt::black);
      }
    }

    //

    return qApp->style()->objectName() == "fusion" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
