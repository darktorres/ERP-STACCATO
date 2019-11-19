#include <QBrush>
#include <QDate>

#include "application.h"
#include "financeiroproxymodel.h"
#include "usersession.h"

FinanceiroProxyModel::FinanceiroProxyModel(SqlRelationalTableModel *model, QObject *parent)
    : QIdentityProxyModel(parent), statusFinanceiro(model->fieldIndex("statusFinanceiro")), prazoEntrega(model->fieldIndex("prazoEntrega")), novoPrazoEntrega(model->fieldIndex("novoPrazoEntrega")) {
  setSourceModel(model);
}

QVariant FinanceiroProxyModel::data(const QModelIndex &proxyIndex, int role) const {
  if (proxyIndex.column() == statusFinanceiro) {
    const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusFinanceiro), Qt::DisplayRole).toString();

    if (status == "PENDENTE") {
      if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
      if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
    }

    if (status == "CONFERIDO") {
      if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
      if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
    }

    if (status == "LIBERADO") {
      if (role == Qt::BackgroundRole) { return QBrush(Qt::green); }
      if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
    }
  }

  if (proxyIndex.column() == prazoEntrega) {
    const QDate prazo = QIdentityProxyModel::data(index(proxyIndex.row(), prazoEntrega), Qt::DisplayRole).toDate();
    const int dias = static_cast<int>(qApp->serverDate().daysTo(prazo));

    if (not prazo.isNull()) {
      if (dias >= 3 and dias < 5) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (dias < 3) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }
  }

  if (proxyIndex.column() == novoPrazoEntrega) {
    const QDate prazo = QIdentityProxyModel::data(index(proxyIndex.row(), novoPrazoEntrega), Qt::DisplayRole).toDate();
    const int dias = static_cast<int>(qApp->serverDate().daysTo(prazo));

    if (not prazo.isNull()) {
      if (dias >= 3 and dias < 5) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (dias < 3) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }
  }

  if (role == Qt::ForegroundRole) {
    const auto tema = UserSession::getSetting("User/tema");

    if (not tema) { return QBrush(Qt::black); }

    return tema->toString() == "claro" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
