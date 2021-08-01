#include "sortfilterproxymodel.h"

#include <QDebug>
#include <QFont>
#include <QSqlRecord>

SortFilterProxyModel::SortFilterProxyModel(QSqlQueryModel *model, QObject *parent)
    : QSortFilterProxyModel(parent), statusColumn(model->record().indexOf("status")), quantColumn(model->record().indexOf("quant")), unidadeColumn(model->record().indexOf("un")) {
  setSourceModel(model);
  setDynamicSortFilter(false);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

SortFilterProxyModel::SortFilterProxyModel(SqlTreeModel *model, QObject *parent)
    : QSortFilterProxyModel(parent), statusColumn(model->fieldIndex("status")), quantColumn(model->fieldIndex("quant")), unidadeColumn(model->fieldIndex("un")) {
  setSourceModel(model);
  setDynamicSortFilter(false);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

QVariant SortFilterProxyModel::data(const QModelIndex &index, int role) const {
  if (statusColumn != -1 and role == Qt::FontRole) {
    const QString status = index.siblingAtColumn(statusColumn).data().toString();

    if (status == "CANCELADA" or status == "CANCELADO" or status == "SUBSTITUIDO") {
      QFont font;
      font.setStrikeOut(true);
      return font;
    }
  }

  // TODO: testar antes de colocar em produção
  //  if (quantColumn != -1 and unidadeColumn != -1 and index.column() == quantColumn and role == Qt::DisplayRole) {
  //    const QString quant = QSortFilterProxyModel::data(index, role).toString();
  //    const QString unidade = index.siblingAtColumn(unidadeColumn).data().toString();

  //    return quant + " " + unidade;
  //  }

  return QSortFilterProxyModel::data(index, role);
}

// TODO: remover coluna 'desativado' de contas pagar/receber e usar status 'cancelado' no lugar
