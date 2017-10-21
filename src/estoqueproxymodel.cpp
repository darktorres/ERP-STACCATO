#include <QApplication>
#include <QBrush>
#include <QStyle>

#include "estoqueproxymodel.h"

EstoqueProxyModel::EstoqueProxyModel(SqlTableModel *model, QObject *parent) : QIdentityProxyModel(parent), quantUpd(model->fieldIndex("quantUpd")) { setSourceModel(model); }

QVariant EstoqueProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole) {
    const int quantUpd = QIdentityProxyModel::data(index(proxyIndex.row(), this->quantUpd), Qt::DisplayRole).toInt();

    // TODO: create a enum
    if (quantUpd == 1) return QBrush(Qt::green);         // Ok
    if (quantUpd == 2) return QBrush(Qt::yellow);        // Quant difere
    if (quantUpd == 3) return QBrush(Qt::red);           // Não encontrado
    if (quantUpd == 4) return QBrush(QColor(0, 190, 0)); // Consumo
    if (quantUpd == 5) return QBrush(Qt::cyan);          // Devolução
  }

  if (role == Qt::ForegroundRole) {
    const int quantUpd = QIdentityProxyModel::data(index(proxyIndex.row(), this->quantUpd), Qt::DisplayRole).toInt();

    // TODO: create a enum
    if (quantUpd == 1) return QBrush(Qt::black); // Ok
    if (quantUpd == 2) return QBrush(Qt::black); // Quant difere
    if (quantUpd == 3) return QBrush(Qt::black); // Não encontrado
    if (quantUpd == 4) return QBrush(Qt::black); // Consumo
    if (quantUpd == 5) return QBrush(Qt::black); // Devolução

    return qApp->style()->objectName() == "fusion" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
