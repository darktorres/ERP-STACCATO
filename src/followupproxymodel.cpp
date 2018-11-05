#include <QBrush>

#include "followupproxymodel.h"
#include "usersession.h"

FollowUpProxyModel::FollowUpProxyModel(SqlRelationalTableModel *model, QObject *parent) : QIdentityProxyModel(parent), semaforoColumn(model->fieldIndex("semaforo")) { setSourceModel(model); }

QVariant FollowUpProxyModel::data(const QModelIndex &proxyIndex, int role) const {
  if (role == Qt::BackgroundRole) {
    const FieldColors semaforo = static_cast<FieldColors>(QIdentityProxyModel::data(index(proxyIndex.row(), semaforoColumn), Qt::DisplayRole).toInt());

    if (semaforo == FieldColors::Quente) { return QBrush(QColor(255, 66, 66)); }
    if (semaforo == FieldColors::Morno) { return QBrush(QColor(255, 170, 0)); }
    if (semaforo == FieldColors::Frio) { return QBrush(QColor(70, 113, 255)); }
  }

  if (role == Qt::ForegroundRole) {
    const FieldColors semaforo = static_cast<FieldColors>(QIdentityProxyModel::data(index(proxyIndex.row(), semaforoColumn), Qt::DisplayRole).toInt());

    if (semaforo == FieldColors::Quente) { return QBrush(Qt::black); }
    if (semaforo == FieldColors::Morno) { return QBrush(Qt::black); }
    if (semaforo == FieldColors::Frio) { return QBrush(Qt::black); }

    const auto tema = UserSession::getSetting("User/tema");

    if (not tema) { return QBrush(Qt::black); }

    return tema->toString() == "claro" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
