#include "nfeproxymodel.h"

#include "application.h"
#include "user.h"

#include <QDebug>
#include <QSqlRecord>

NFeProxyModel::NFeProxyModel(QSqlQueryModel *model, QObject *parent)
    : SortFilterProxyModel(model, parent), dataColumn(model->record().indexOf("dataDistribuicao")), statusColumn(model->record().indexOf("statusDistribuicao")) {
  if (dataColumn == -1 or statusColumn == -1) { throw RuntimeException("NFeProxyModel Coluna -1!"); }
}

QVariant NFeProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (statusColumn != -1 and dataColumn != -1 and (role == Qt::BackgroundRole or role == Qt::ForegroundRole)) {
    const QString statusEvento = proxyIndex.siblingAtColumn(statusColumn).data().toString();
    const QDate dataEvento = proxyIndex.siblingAtColumn(dataColumn).data().toDate();

    // prazo limite é de 180 dias

    if (statusEvento == "CANCELADA" and role == Qt::BackgroundRole) { return QBrush(Qt::gray); }
    if (statusEvento == "CANCELADA" and role == Qt::ForegroundRole) { return QBrush(Qt::black); }

    if (statusEvento == "CONFIRMAÇÃO" and role == Qt::BackgroundRole) { return QBrush(Qt::green); }
    if (statusEvento == "CONFIRMAÇÃO" and role == Qt::ForegroundRole) { return QBrush(Qt::black); }

    if (statusEvento == "CIÊNCIA") {
      if (qApp->serverDate() > dataEvento.addDays(60) and role == Qt::BackgroundRole) { return QBrush(Qt::red); }
      if (qApp->serverDate() > dataEvento.addDays(60) and role == Qt::ForegroundRole) { return QBrush(Qt::black); }

      if (qApp->serverDate() > dataEvento.addDays(30) and role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
      if (qApp->serverDate() > dataEvento.addDays(30) and role == Qt::ForegroundRole) { return QBrush(Qt::black); }
    }
  }

  if (role == Qt::ForegroundRole) {
    const QString tema = User::getSetting("User/tema").toString();

    return (tema == "escuro") ? QBrush(Qt::white) : QBrush(Qt::black);
  }

  return SortFilterProxyModel::data(proxyIndex, role);
}
