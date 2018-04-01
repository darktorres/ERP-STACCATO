#include <QApplication>
#include <QBrush>
#include <QStyle>

#include "estoqueproxymodel.h"

EstoqueProxyModel::EstoqueProxyModel(SqlRelationalTableModel *model, QObject *parent) : QIdentityProxyModel(parent), quantUpdIndex(model->fieldIndex("quantUpd")) { setSourceModel(model); }

QVariant EstoqueProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole) {
    const Status quantUpd = static_cast<Status>(QIdentityProxyModel::data(index(proxyIndex.row(), quantUpdIndex), Qt::DisplayRole).toInt());

    if (quantUpd == Status::Ok) { return QBrush(Qt::green); }
    if (quantUpd == Status::QuantDifere) { return QBrush(Qt::yellow); }
    if (quantUpd == Status::NaoEncontrado) { return QBrush(Qt::red); }
    if (quantUpd == Status::Consumo) { return QBrush(QColor(0, 190, 0)); }
    if (quantUpd == Status::Devolucao) { return QBrush(Qt::cyan); }
  }

  if (role == Qt::ForegroundRole) {

    // those paint the text as black if the background is colored

    const Status quantUpd = static_cast<Status>(QIdentityProxyModel::data(index(proxyIndex.row(), quantUpdIndex), Qt::DisplayRole).toInt());

    if (quantUpd == Status::Ok) { return QBrush(Qt::black); }
    if (quantUpd == Status::QuantDifere) { return QBrush(Qt::black); }
    if (quantUpd == Status::NaoEncontrado) { return QBrush(Qt::black); }
    if (quantUpd == Status::Consumo) { return QBrush(Qt::black); }
    if (quantUpd == Status::Devolucao) { return QBrush(Qt::black); }

    //

    return qApp->style()->objectName() == "fusion" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
