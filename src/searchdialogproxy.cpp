#include <QApplication>
#include <QBrush>
#include <QDate>
#include <QStyle>

#include "searchdialogproxy.h"

SearchDialogProxy::SearchDialogProxy(SqlTableModel *model, QObject *parent)
    : QIdentityProxyModel(parent), estoque(model->fieldIndex("estoque")), promocao(model->fieldIndex("promocao")), descontinuado(model->fieldIndex("descontinuado")),
      validade(model->fieldIndex("validadeProdutos")) {
  setSourceModel(model);
}

QVariant SearchDialogProxy::data(const QModelIndex &proxyIndex, int role) const {
  if (role == Qt::BackgroundRole) {
    const bool descontinuado = QIdentityProxyModel::data(index(proxyIndex.row(), this->descontinuado), Qt::DisplayRole).toBool();

    if (descontinuado == true) return QBrush(Qt::red); // descontinuado

    const bool estoque = QIdentityProxyModel::data(index(proxyIndex.row(), this->estoque), Qt::DisplayRole).toBool();
    const bool promocao = QIdentityProxyModel::data(index(proxyIndex.row(), this->promocao), Qt::DisplayRole).toBool();

    if (estoque == true) return QBrush(Qt::yellow); // estoque
    if (promocao == true) return QBrush(Qt::green); // promocao

    if (proxyIndex.column() == this->validade) {
      const QDate validade = QIdentityProxyModel::data(index(proxyIndex.row(), this->validade), Qt::DisplayRole).toDate();

      if (validade < QDate::currentDate()) return QBrush(Qt::red);
    }
  }

  if (role == Qt::ForegroundRole) {

    // those paint the text as black if the background is colored

    const bool descontinuado = QIdentityProxyModel::data(index(proxyIndex.row(), this->descontinuado), Qt::DisplayRole).toBool();

    if (descontinuado == true) return QBrush(Qt::black);

    const bool estoque = QIdentityProxyModel::data(index(proxyIndex.row(), this->estoque), Qt::DisplayRole).toBool();
    const bool promocao = QIdentityProxyModel::data(index(proxyIndex.row(), this->promocao), Qt::DisplayRole).toBool();

    if (estoque == true) return QBrush(Qt::black);
    if (promocao == true) return QBrush(Qt::black);

    if (proxyIndex.column() == this->validade) {
      const QDate validade = QIdentityProxyModel::data(index(proxyIndex.row(), this->validade), Qt::DisplayRole).toDate();

      if (validade < QDate::currentDate()) return QBrush(Qt::black);
    }

    //

    return qApp->style()->objectName() == "fusion" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}
