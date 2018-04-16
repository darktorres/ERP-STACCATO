#include <QApplication>
#include <QBrush>
#include <QDate>
#include <QStyle>

#include "searchdialogproxymodel.h"

SearchDialogProxyModel::SearchDialogProxyModel(SqlRelationalTableModel *model, QObject *parent)
    : QIdentityProxyModel(parent), estoque(model->fieldIndex("estoque")), promocao(model->fieldIndex("promocao")), descontinuado(model->fieldIndex("descontinuado")),
      validade(model->fieldIndex("validadeProdutos")) {
  setSourceModel(model);
}

QVariant SearchDialogProxyModel::data(const QModelIndex &proxyIndex, int role) const {
  if (role == Qt::BackgroundRole) {
    const bool descontinuado = QIdentityProxyModel::data(index(proxyIndex.row(), this->descontinuado), Qt::DisplayRole).toBool();

    if (descontinuado == true) { return QBrush(Qt::red); } // descontinuado

    const int estoque = QIdentityProxyModel::data(index(proxyIndex.row(), this->estoque), Qt::DisplayRole).toInt();
    const bool promocao = QIdentityProxyModel::data(index(proxyIndex.row(), this->promocao), Qt::DisplayRole).toBool();

    if (estoque == 1) { return QBrush(Qt::yellow); }
    if (estoque == 2) { return QBrush(Qt::blue); }
    if (promocao == true) { return QBrush(Qt::green); } // promocao

    if (proxyIndex.column() == this->validade) {
      const QDate validade = QIdentityProxyModel::data(index(proxyIndex.row(), this->validade), Qt::DisplayRole).toDate();

      if (validade < QDate::currentDate()) { return QBrush(Qt::red); }
    }
  }

  if (role == Qt::ForegroundRole) {

    // those paint the text as black if the background is colored

    const bool descontinuado = QIdentityProxyModel::data(index(proxyIndex.row(), this->descontinuado), Qt::DisplayRole).toBool();

    if (descontinuado == true) { return QBrush(Qt::black); }

    const int estoque = QIdentityProxyModel::data(index(proxyIndex.row(), this->estoque), Qt::DisplayRole).toInt();
    const bool promocao = QIdentityProxyModel::data(index(proxyIndex.row(), this->promocao), Qt::DisplayRole).toBool();

    if (estoque == 1) { return QBrush(Qt::black); }
    if (estoque == 2) { return QBrush(Qt::white); }
    if (promocao == true) { return QBrush(Qt::black); }

    if (proxyIndex.column() == this->validade) {
      const QDate validade = QIdentityProxyModel::data(index(proxyIndex.row(), this->validade), Qt::DisplayRole).toDate();

      if (validade < QDate::currentDate()) { return QBrush(Qt::black); }
    }

    // -------------------------------------------------------------------------

    return qApp->style()->objectName() == "fusion" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}

// TODO: posteriormente remover o azul da promocao 'staccato off'
