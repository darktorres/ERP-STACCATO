#include <QDebug>
#include <QMenu>

#include "application.h"
#include "treeview.h"

TreeView::TreeView(QWidget *parent) : QTreeView(parent) {
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &QTreeView::expanded, this, &TreeView::resizeAllColumns);
  connect(this, &QTreeView::collapsed, this, &TreeView::resizeAllColumns);
  connect(this, &QWidget::customContextMenuRequested, this, &TreeView::showContextMenu);
}

void TreeView::showContextMenu(const QPoint &pos) {
  QMenu contextMenu;

  QAction actionExpand("Expandir tudo", this);
  connect(&actionExpand, &QAction::triggered, this, &TreeView::expandAll);
  contextMenu.addAction(&actionExpand);

  QAction actionCollapse("Recolher tudo", this);
  connect(&actionCollapse, &QAction::triggered, this, &TreeView::collapseAll);
  contextMenu.addAction(&actionCollapse);

  contextMenu.exec(mapToGlobal(pos));
}

void TreeView::hideColumn(const QString &column) { QTreeView::hideColumn(columnIndex(column)); }

void TreeView::setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) { QTreeView::setItemDelegateForColumn(columnIndex(column), delegate); }

void TreeView::setModel(QAbstractItemModel *model) {
  if (auto temp = qobject_cast<SqlTreeModel *>(model); temp and temp->proxyModel) {
    QTreeView::setModel(temp->proxyModel);
  } else {
    QTreeView::setModel(model);
  }

  baseModel = qobject_cast<SqlTreeModel *>(model);

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

// TODO: copy right-click menu from TableView
