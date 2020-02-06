#include "importaprodutosproxymodel.h"

#include "usersession.h"

#include <QBrush>

ImportaProdutosProxyModel::ImportaProdutosProxyModel(SqlTableModel *model, QObject *parent) : QIdentityProxyModel(parent), descontinuadoColumn(model->fieldIndex("descontinuado")) {
  setSourceModel(model);
}

QVariant ImportaProdutosProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole or role == Qt::ForegroundRole) {
    if (descontinuadoColumn != -1) {
      const bool descontinuado = proxyIndex.siblingAtColumn(descontinuadoColumn).data().toBool();

      if (descontinuado) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::cyan); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      for (int column = 0, columns = columnCount(); column < columns; ++column) {
        if (proxyIndex.column() == column) {
          const Status value = static_cast<Status>(proxyIndex.siblingAtColumn(proxyIndex.column() + 1).data().toInt());

          if (value == Status::Novo) {
            if (role == Qt::BackgroundRole) { return QBrush(Qt::green); }
            if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
          }

          if (value == Status::Atualizado) {
            if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
            if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
          }

          if (value == Status::ForaPadrao) {
            if (role == Qt::BackgroundRole) { return QBrush(Qt::gray); }
            if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
          }

          if (value == Status::Errado) {
            if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
            if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
          }
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
