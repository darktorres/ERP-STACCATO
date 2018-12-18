#include <QBrush>

#include "usersession.h"
#include "vendaproxymodel.h"

VendaProxyModel::VendaProxyModel(SqlRelationalTableModel *model, QObject *parent)
    : QIdentityProxyModel(parent), diasRestantesIndex(model->fieldIndex("Dias restantes")), statusIndex(model->fieldIndex("Status")), financeiroIndex(model->fieldIndex("statusFinanceiro")) {
  setSourceModel(model);
}

QVariant VendaProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (statusIndex != -1) {
    const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();

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
    const int dias = QIdentityProxyModel::data(index(proxyIndex.row(), diasRestantesIndex), Qt::DisplayRole).toInt();
    const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();

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
    const QString financeiro = QIdentityProxyModel::data(index(proxyIndex.row(), financeiroIndex), Qt::DisplayRole).toString();

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
    const auto tema = UserSession::getSetting("User/tema");

    if (not tema) { return QBrush(Qt::black); }

    return tema->toString() == "claro" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
