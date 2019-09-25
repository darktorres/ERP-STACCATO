#include <QBrush>
#include <QDate>
#include <QDebug>

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
      const auto adjustedIndex = proxyIndex.model()->index(proxyIndex.row(), descontinuadoColumn, proxyIndex.parent());

      const bool descontinuado = QSortFilterProxyModel::data(adjustedIndex).toBool();

      if (descontinuado) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (estoqueColumn != -1) {
      const auto adjustedIndex = proxyIndex.model()->index(proxyIndex.row(), estoqueColumn, proxyIndex.parent());

      const bool estoque = QSortFilterProxyModel::data(adjustedIndex).toBool();

      if (estoque) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (promocaoColumn != -1) {
      const auto adjustedIndex = proxyIndex.model()->index(proxyIndex.row(), promocaoColumn, proxyIndex.parent());

      const int promocao = QSortFilterProxyModel::data(adjustedIndex).toInt();

      if (promocao == 1) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::green); } // promocao
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (promocao == 2) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::blue); } // staccato OFF
        if (role == Qt::ForegroundRole) { return QBrush(Qt::white); }
      }
    }

    if (proxyIndex.column() == validadeColumn) {
      const auto adjustedIndex = proxyIndex.model()->index(proxyIndex.row(), validadeColumn, proxyIndex.parent());

      const QDate validade = QSortFilterProxyModel::data(adjustedIndex).toDate();
      const bool expirado = validade < QDate::currentDate();

      if (expirado) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (role == Qt::ForegroundRole) {
      const auto tema = UserSession::getSetting("User/tema");

      if (not tema) { return QBrush(Qt::black); }

      return tema->toString() == "claro" ? QBrush(Qt::black) : QBrush(Qt::white);
    }
  }

  return QSortFilterProxyModel::data(proxyIndex, role);
}
