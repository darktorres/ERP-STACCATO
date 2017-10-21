#include <QApplication>
#include <QBrush>
#include <QDebug>
#include <QStyle>

#include "followupproxymodel.h"

FollowUpProxyModel::FollowUpProxyModel(SqlTableModel *model, QObject *parent) : QIdentityProxyModel(parent), semaforo(model->fieldIndex("semaforo")) { setSourceModel(model); }

QVariant FollowUpProxyModel::data(const QModelIndex &proxyIndex, int role) const {
  if (role == Qt::BackgroundRole) {
    const int semaforo = QIdentityProxyModel::data(index(proxyIndex.row(), this->semaforo), Qt::DisplayRole).toInt();

    if (semaforo == Quente) return QBrush(QColor(255, 66, 66));
    if (semaforo == Morno) return QBrush(QColor(255, 170, 0));
    if (semaforo == Frio) return QBrush(QColor(70, 113, 255));
  }

  if (role == Qt::ForegroundRole) {
    const int semaforo = QIdentityProxyModel::data(index(proxyIndex.row(), this->semaforo), Qt::DisplayRole).toInt();

    if (semaforo == Quente) return QBrush(Qt::black);
    if (semaforo == Morno) return QBrush(Qt::black);
    if (semaforo == Frio) return QBrush(Qt::black);

    return qApp->style()->objectName() == "fusion" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
