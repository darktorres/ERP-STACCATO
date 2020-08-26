#include "vendaproxymodel.h"

#include "usersession.h"

#include <QBrush>
#include <QSqlRecord>

VendaProxyModel::VendaProxyModel(QSqlQueryModel *model, QObject *parent)
    : QIdentityProxyModel(parent), diasRestantesIndex(model->record().indexOf("Dias restantes")), statusIndex(model->record().indexOf("Status")),
      financeiroIndex(model->record().indexOf("statusFinanceiro")) {
  setSourceModel(model);
}

QVariant VendaProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole or role == Qt::ForegroundRole) {
    if (statusIndex != -1) {
      const QString status = proxyIndex.siblingAtColumn(statusIndex).data().toString();

      if (status == "ENTREGUE") {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::green); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (status == "CANCELADO" or status == "DEVOLVIDO" or status == "PERDIDO") {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (proxyIndex.column() == diasRestantesIndex) {
      const int dias = proxyIndex.siblingAtColumn(diasRestantesIndex).data().toInt();
      const QString status = proxyIndex.siblingAtColumn(statusIndex).data().toString();

      if (status == "ENTREGUE") {
        if (role == Qt::DisplayRole) { return QVariant(); }
      }

      if (dias >= 5) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::green); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      } else if (dias >= 3) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      } else {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (proxyIndex.column() == financeiroIndex) {
      const QString financeiro = proxyIndex.siblingAtColumn(financeiroIndex).data().toString();

      if (financeiro == "PENDENTE") {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (financeiro == "CONFERIDO") {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (financeiro == "LIBERADO") {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::green); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (role == Qt::ForegroundRole) {
      const QString tema = UserSession::getSetting("User/tema").value_or("claro").toString();

      return (tema == "claro") ? QBrush(Qt::black) : QBrush(Qt::white);
    }
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
