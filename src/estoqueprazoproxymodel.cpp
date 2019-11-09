#include <QBrush>
#include <QDate>

#include "application.h"
#include "estoqueprazoproxymodel.h"
#include "usersession.h"

EstoquePrazoProxyModel::EstoquePrazoProxyModel(SqlRelationalTableModel *model, QObject *parent) : QIdentityProxyModel(parent), prazoEntregaColumn(model->fieldIndex("prazoEntrega")) {
  setSourceModel(model);
}

QVariant EstoquePrazoProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (proxyIndex.column() == prazoEntregaColumn) {
    const QDate prazo = QIdentityProxyModel::data(index(proxyIndex.row(), prazoEntregaColumn), Qt::DisplayRole).toDate();
    const bool atrasado = not prazo.isNull() and prazo < qApp->serverDateTime().date();

    if (atrasado) {
      if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
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
