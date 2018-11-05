#include <QBrush>
#include <QDate>

#include "searchdialogproxymodel.h"
#include "usersession.h"

SearchDialogProxyModel::SearchDialogProxyModel(SqlRelationalTableModel *model, QObject *parent)
    : QIdentityProxyModel(parent), estoqueColumn(model->fieldIndex("estoque")), promocaoColumn(model->fieldIndex("promocao")), descontinuadoColumn(model->fieldIndex("descontinuado")),
      validadeColumn(model->fieldIndex("validadeProdutos")) {
  setSourceModel(model);
}

QVariant SearchDialogProxyModel::data(const QModelIndex &proxyIndex, int role) const {
  if (role == Qt::BackgroundRole) {
    const bool descontinuado = QIdentityProxyModel::data(index(proxyIndex.row(), descontinuadoColumn), Qt::DisplayRole).toBool();

    if (descontinuado) { return QBrush(Qt::red); } // descontinuado

    const int estoque = QIdentityProxyModel::data(index(proxyIndex.row(), estoqueColumn), Qt::DisplayRole).toInt();
    const bool promocao = QIdentityProxyModel::data(index(proxyIndex.row(), promocaoColumn), Qt::DisplayRole).toBool();

    if (estoque == 1) { return QBrush(Qt::yellow); }
    if (estoque == 2) { return QBrush(Qt::blue); }
    if (promocao) { return QBrush(Qt::green); } // promocao

    if (proxyIndex.column() == validadeColumn) {
      const QDate validade = QIdentityProxyModel::data(index(proxyIndex.row(), validadeColumn), Qt::DisplayRole).toDate();

      if (validade < QDate::currentDate()) { return QBrush(Qt::red); }
    }
  }

  if (role == Qt::ForegroundRole) {

    // those paint the text as black if the background is colored

    const bool descontinuado = QIdentityProxyModel::data(index(proxyIndex.row(), descontinuadoColumn), Qt::DisplayRole).toBool();

    if (descontinuado) { return QBrush(Qt::black); }

    const int estoque = QIdentityProxyModel::data(index(proxyIndex.row(), estoqueColumn), Qt::DisplayRole).toInt();
    const bool promocao = QIdentityProxyModel::data(index(proxyIndex.row(), promocaoColumn), Qt::DisplayRole).toBool();

    if (estoque == 1) { return QBrush(Qt::black); }
    if (estoque == 2) { return QBrush(Qt::white); }
    if (promocao) { return QBrush(Qt::black); }

    if (proxyIndex.column() == validadeColumn) {
      const QDate validade = QIdentityProxyModel::data(index(proxyIndex.row(), validadeColumn), Qt::DisplayRole).toDate();

      if (validade < QDate::currentDate()) { return QBrush(Qt::black); }
    }

    // -------------------------------------------------------------------------

    const auto tema = UserSession::getSetting("User/tema");

    if (not tema) { return QBrush(Qt::black); }

    return tema->toString() == "claro" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}

// TODO: posteriormente remover o azul da promocao 'staccato off'
