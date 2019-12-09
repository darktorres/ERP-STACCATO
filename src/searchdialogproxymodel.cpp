#include <QBrush>
#include <QDate>
#include <QDebug>

#include "application.h"
#include "searchdialogproxymodel.h"
#include "usersession.h"

SearchDialogProxyModel::SearchDialogProxyModel(SqlRelationalTableModel *model, QObject *parent)
    : SortFilterProxyModel(model, parent), estoqueColumn(model->fieldIndex("estoque", true)), promocaoColumn(model->fieldIndex("promocao", true)),
      descontinuadoColumn(model->fieldIndex("descontinuado", true)), validadeColumn(model->fieldIndex("validadeProdutos", true)) {}

SearchDialogProxyModel::SearchDialogProxyModel(SqlTreeModel *model, QObject *parent)
    : SortFilterProxyModel(model, parent), estoqueColumn(model->fieldIndex("estoque")), promocaoColumn(model->fieldIndex("promocao")) {}

QVariant SearchDialogProxyModel::data(const QModelIndex &proxyIndex, int role) const {
  if (role == Qt::BackgroundRole or role == Qt::ForegroundRole) {
    if (descontinuadoColumn != -1) {
      const bool descontinuado = proxyIndex.siblingAtColumn(descontinuadoColumn).data().toBool();

      if (descontinuado) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (estoqueColumn != -1 and promocaoColumn != -1) {
      const bool estoque = proxyIndex.siblingAtColumn(estoqueColumn).data().toBool();
      const int promocao = proxyIndex.siblingAtColumn(promocaoColumn).data().toInt();

      if (estoque and promocao == 0) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (estoque and promocao == 2) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::blue); } // BLACK NOVEMBER
        if (role == Qt::ForegroundRole) { return QBrush(Qt::white); }
      }

      if (not estoque and promocao == 1) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::green); } // promocao
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (proxyIndex.column() == validadeColumn) {
      const QDate validade = proxyIndex.siblingAtColumn(validadeColumn).data().toDate();
      const bool expirado = validade < qApp->serverDate();

      if (expirado) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (role == Qt::BackgroundRole) {
      if (not proxyIndex.model()->hasChildren(proxyIndex) and proxyIndex.parent().isValid()) { return QBrush(Qt::gray); }
    }

    if (role == Qt::ForegroundRole) {
      const auto tema = UserSession::getSetting("User/tema");

      if (not tema) { return QBrush(Qt::black); }

      return tema->toString() == "claro" ? QBrush(Qt::black) : QBrush(Qt::white);
    }
  }

  return QSortFilterProxyModel::data(proxyIndex, role);
}

// TODO: posteriormente remover o azul da promocao 'BLACK NOVEMBER'
