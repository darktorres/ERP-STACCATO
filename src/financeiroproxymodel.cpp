#include "financeiroproxymodel.h"

#include "application.h"
#include "usersession.h"

#include <QBrush>
#include <QDate>

FinanceiroProxyModel::FinanceiroProxyModel(SqlRelationalTableModel *model, QObject *parent)
    : QIdentityProxyModel(parent), statusFinanceiro(model->fieldIndex("statusFinanceiro")), prazoEntrega(model->fieldIndex("prazoEntrega")), novoPrazoEntrega(model->fieldIndex("novoPrazoEntrega")) {
  setSourceModel(model);
}

QVariant FinanceiroProxyModel::data(const QModelIndex &proxyIndex, int role) const {
  if (role == Qt::BackgroundRole or role == Qt::ForegroundRole) {
    if (proxyIndex.column() == statusFinanceiro) {
      const QString status = proxyIndex.siblingAtColumn(statusFinanceiro).data().toString();

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
      const QDate prazo = proxyIndex.siblingAtColumn(prazoEntrega).data().toDate();
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
      const QDate prazo = proxyIndex.siblingAtColumn(novoPrazoEntrega).data().toDate();
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
      const QString tema = UserSession::getSetting("User/tema").value_or("claro").toString();

      return (tema == "claro") ? QBrush(Qt::black) : QBrush(Qt::white);
    }
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
