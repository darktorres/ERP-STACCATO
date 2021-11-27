#include "followupproxymodel.h"

#include "user.h"

#include <QSqlRecord>

FollowUpProxyModel::FollowUpProxyModel(QSqlQueryModel *model, QObject *parent) : QIdentityProxyModel(parent), semaforoColumn(model->record().indexOf("semaforo")) { setSourceModel(model); }

QVariant FollowUpProxyModel::data(const QModelIndex &proxyIndex, int role) const {
  if (role == Qt::BackgroundRole or role == Qt::ForegroundRole) {
    if (semaforoColumn != -1) {
      const FieldColors semaforo = static_cast<FieldColors>(proxyIndex.siblingAtColumn(semaforoColumn).data().toInt());

      if (semaforo == FieldColors::Quente) {
        if (role == Qt::BackgroundRole) { return QBrush(QColor(255, 66, 66)); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (semaforo == FieldColors::Morno) {
        if (role == Qt::BackgroundRole) { return QBrush(QColor(255, 170, 0)); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (semaforo == FieldColors::Frio) {
        if (role == Qt::BackgroundRole) { return QBrush(QColor(70, 113, 255)); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (role == Qt::ForegroundRole) {
      const QString tema = User::getSetting("User/tema").toString();

      return (tema == "escuro") ? QBrush(Qt::white) : QBrush(Qt::black);
    }
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
