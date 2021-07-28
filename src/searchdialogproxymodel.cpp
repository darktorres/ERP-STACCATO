#include "searchdialogproxymodel.h"

#include "application.h"
#include "user.h"

#include <QBrush>
#include <QDate>
#include <QDebug>
#include <QSqlRecord>

SearchDialogProxyModel::SearchDialogProxyModel(QSqlQueryModel *model, QObject *parent)
    : SortFilterProxyModel(model, parent), descontinuadoColumn(model->record().indexOf("descontinuado")), estoqueColumn(model->record().indexOf("estoque")),
      promocaoColumn(model->record().indexOf("promocao")) {}

SearchDialogProxyModel::SearchDialogProxyModel(SqlTreeModel *model, QObject *parent)
    : SortFilterProxyModel(model, parent), descontinuadoColumn(model->fieldIndex("descontinuado")), estoqueColumn(model->fieldIndex("estoque")), promocaoColumn(model->fieldIndex("promocao")) {}

QVariant SearchDialogProxyModel::data(const QModelIndex &proxyIndex, int role) const {
  if (role == Qt::BackgroundRole or role == Qt::ForegroundRole) {
    const bool isChild = not proxyIndex.model()->hasChildren(proxyIndex) and proxyIndex.parent().isValid();

    if (isChild) {
      const QString tema = User::getSetting("User/tema").toString();

      if (role == Qt::BackgroundRole) { return (tema == "escuro") ? QBrush(Qt::darkGray) : QBrush(Qt::gray); }
      if (role == Qt::ForegroundRole) { return (tema == "escuro") ? QBrush(Qt::white) : QBrush(Qt::black); }
    }

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

      // estoque
      if (estoque and promocao == 0) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      // STACCATO OFF
      if (estoque and promocao == 2) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::blue); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::white); }
      }

      // promocao
      if (not estoque and promocao == 1) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::green); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }
  }

  if (role == Qt::ForegroundRole) {
    const QString tema = User::getSetting("User/tema").toString();

    return (tema == "escuro") ? QBrush(Qt::white) : QBrush(Qt::black);
  }

  return SortFilterProxyModel::data(proxyIndex, role);
}

bool SearchDialogProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
  if (source_left.parent().isValid()) { return false; } // disable sorting for childs

  return QSortFilterProxyModel::lessThan(source_left, source_right);
}
