#include "treeview.h"
#include "application.h"

#include <QDebug>

TreeView::TreeView(QWidget *parent) : QTreeView(parent) {
  connect(this, &QTreeView::expanded, this, &TreeView::resizeAllColumns);
  connect(this, &QTreeView::collapsed, this, &TreeView::resizeAllColumns);
}

void TreeView::hideColumn(const QString &column) { QTreeView::hideColumn(columnIndex(column)); }

void TreeView::setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) { QTreeView::setItemDelegateForColumn(columnIndex(column), delegate); }

void TreeView::setModel(QIdentityProxyModel *model) {
  baseModel = static_cast<SqlTreeModel *>(model->sourceModel());

  setModel(static_cast<QAbstractItemModel *>(model));
}

void TreeView::setModel(QSortFilterProxyModel *model) {
  baseModel = static_cast<SqlTreeModel *>(model->sourceModel());

  setModel(static_cast<QAbstractItemModel *>(model));
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
void TreeView::setModel(QTransposeProxyModel *model) {
  baseModel = static_cast<SqlTreeModel *>(model->sourceModel());

  setModel(static_cast<QAbstractItemModel *>(model));
}
#endif

void TreeView::setModel(SqlTreeModel *model) {
  baseModel = model;

  setModel(static_cast<QAbstractItemModel *>(model));
}

void TreeView::setModel(QAbstractItemModel *model) {
  QTreeView::setModel(model);

  if (not baseModel) { qApp->enqueueError("Sem baseModel!"); }

  resizeAllColumns();
}

void TreeView::resizeAllColumns() {
  for (int col = 0; col < model()->columnCount(); ++col) { resizeColumnToContents(col); }
}

int TreeView::columnIndex(const QString &column) const {
  int columnIndex = -1;

  if (baseModel) { columnIndex = baseModel->fieldIndex(column); }

  if (columnIndex == -1 and column != "created" and column != "lastUpdated") { qApp->enqueueError("Coluna '" + column + "' não encontrada!"); }

  return columnIndex;
}
