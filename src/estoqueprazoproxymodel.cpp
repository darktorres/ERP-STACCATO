#include <QBrush>
#include <QDate>

#include "estoqueprazoproxymodel.h"
#include "usersession.h"

EstoquePrazoProxyModel::EstoquePrazoProxyModel(SqlRelationalTableModel *model, QObject *parent) : QIdentityProxyModel(parent), dias(model->fieldIndex("prazoEntrega")) { setSourceModel(model); }

QVariant EstoquePrazoProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole) {
    if (proxyIndex.column() == this->dias) {
      const QDate prazo = QIdentityProxyModel::data(index(proxyIndex.row(), this->dias), Qt::DisplayRole).toDate();

      if (prazo < QDate::currentDate() and not prazo.isNull()) { return QBrush(Qt::red); }
    }
  }

  if (role == Qt::ForegroundRole) {

    // those paint the text as black if the background is colored

    if (proxyIndex.column() == this->dias) {
      const QDate prazo = QIdentityProxyModel::data(index(proxyIndex.row(), this->dias), Qt::DisplayRole).toDate();

      if (prazo < QDate::currentDate() and not prazo.isNull()) { return QBrush(Qt::black); }
    }

    // -------------------------------------------------------------------------

    const auto tema = UserSession::getSetting("User/tema");

    if (not tema) { return QBrush(Qt::black); }

    return tema->toString() == "claro" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
