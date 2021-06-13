#include "vendaproxymodel.h"

#include "user.h"

#include <QBrush>
#include <QSqlRecord>

VendaProxyModel::VendaProxyModel(QSqlQueryModel *model, QObject *parent)
    : QIdentityProxyModel(parent), diasRestantesIndex(model->record().indexOf("Dias restantes")), financeiroIndex(model->record().indexOf("statusFinanceiro")),
      statusIndex(model->record().indexOf("Status")) {
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
      const QString tema = User::getSetting("User/tema").toString();

      return (tema == "escuro") ? QBrush(Qt::white) : QBrush(Qt::black);
    }
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
