#include "estoqueproxymodel.h"

#include "application.h"
#include "user.h"

#include <QBrush>
#include <QSqlRecord>

EstoqueProxyModel::EstoqueProxyModel(QSqlQueryModel *model, QObject *parent) : SortFilterProxyModel(model, parent), quantUpdColumn(model->record().indexOf("quantUpd")) {
  if (quantUpdColumn == -1) { throw RuntimeException("EstoqueProxyModel Coluna -1!"); }
}

QVariant EstoqueProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (quantUpdColumn != -1 and (role == Qt::BackgroundRole or role == Qt::ForegroundRole)) {
    const Status quantUpd = static_cast<Status>(proxyIndex.siblingAtColumn(quantUpdColumn).data().toInt());

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
    const QString tema = User::getSetting("User/tema").toString();

    return (tema == "escuro") ? QBrush(Qt::white) : QBrush(Qt::black);
  }

  return SortFilterProxyModel::data(proxyIndex, role);
}
