#include <QBrush>

#include "estoqueproxymodel.h"
#include "usersession.h"

EstoqueProxyModel::EstoqueProxyModel(SqlRelationalTableModel *model, QObject *parent) : SortFilterProxyModel(model, parent), quantUpdColumn(model->fieldIndex("quantUpd")) {}

QVariant EstoqueProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole or role == Qt::ForegroundRole) {
    if (quantUpdColumn != -1) {
      const auto adjustedIndex = proxyIndex.model()->index(proxyIndex.row(), quantUpdColumn, proxyIndex.parent());

      const Status quantUpd = static_cast<Status>(QSortFilterProxyModel::data(adjustedIndex).toInt());

      if (quantUpd == Status::Ok) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::green); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (quantUpd == Status::QuantDifere) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (quantUpd == Status::NaoEncontrado) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (quantUpd == Status::Consumo) {
        if (role == Qt::BackgroundRole) { return QBrush(QColor(0, 190, 0)); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (quantUpd == Status::Devolucao) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::cyan); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (role == Qt::ForegroundRole) {
      const auto tema = UserSession::getSetting("User/tema");

      if (not tema) { return QBrush(Qt::black); }

      return tema->toString() == "claro" ? QBrush(Qt::black) : QBrush(Qt::white);
    }
  }

  return QSortFilterProxyModel::data(proxyIndex, role);
}
