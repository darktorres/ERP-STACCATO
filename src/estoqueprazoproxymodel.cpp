#include "estoqueprazoproxymodel.h"

#include "application.h"
#include "user.h"

#include <QBrush>
#include <QDate>
#include <QSqlRecord>

EstoquePrazoProxyModel::EstoquePrazoProxyModel(QSqlQueryModel *model, QObject *parent) : QIdentityProxyModel(parent), prazoEntregaColumn(model->record().indexOf("prazoEntrega")) {
  setSourceModel(model);
}

QVariant EstoquePrazoProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole or role == Qt::ForegroundRole) {
    if (proxyIndex.column() == prazoEntregaColumn) {
      const QDate prazo = proxyIndex.siblingAtColumn(prazoEntregaColumn).data().toDate();
      const bool atrasado = (not prazo.isNull() and prazo < qApp->serverDate());

      if (atrasado) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
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
