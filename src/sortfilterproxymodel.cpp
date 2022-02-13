#include "sortfilterproxymodel.h"

#include <QDebug>
#include <QFont>
#include <QSqlRecord>

SortFilterProxyModel::SortFilterProxyModel(QSqlQueryModel *model, QObject *parent) : QSortFilterProxyModel(parent), statusColumn(model->record().indexOf("status")) {
  setSourceModel(model);
  setDynamicSortFilter(false);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

SortFilterProxyModel::SortFilterProxyModel(SqlTreeModel *model, QObject *parent) : QSortFilterProxyModel(parent), statusColumn(model->fieldIndex("status")) {
  setSourceModel(model);
  setDynamicSortFilter(false);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

// TODO: remover coluna 'desativado' de contas pagar/receber e usar status 'cancelado' no lugar
