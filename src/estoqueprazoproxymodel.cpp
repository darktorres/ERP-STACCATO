#include "estoqueprazoproxymodel.h"

#include "application.h"
#include "usersession.h"

#include <QBrush>
#include <QDate>

EstoquePrazoProxyModel::EstoquePrazoProxyModel(SqlTableModel *model, QObject *parent) : QIdentityProxyModel(parent), prazoEntregaColumn(model->fieldIndex("prazoEntrega")) {
  setSourceModel(model);
}

QVariant EstoquePrazoProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole or role == Qt::ForegroundRole) {
    if (proxyIndex.column() == prazoEntregaColumn) {
      const QDate prazo = proxyIndex.siblingAtColumn(prazoEntregaColumn).data().toDate();
      const bool atrasado = not prazo.isNull() and prazo < qApp->serverDate();

      if (atrasado) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
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
