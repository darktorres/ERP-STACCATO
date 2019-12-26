#include <QDebug>
#include <QMenu>
#include <QScrollBar>

#include "application.h"
#include "treeview.h"

TreeView::TreeView(QWidget *parent) : QTreeView(parent) {
  setContextMenuPolicy(Qt::CustomContextMenu);

  setConnections();

  setAlternatingRowColors(true);
  setUniformRowHeights(true);
  setSortingEnabled(true);
}

void TreeView::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(this, &QTreeView::expanded, this, &TreeView::resizeAllColumns, connectionType);
  connect(this, &QTreeView::collapsed, this, &TreeView::resizeAllColumns, connectionType);
  connect(this, &QWidget::customContextMenuRequested, this, &TreeView::showContextMenu, connectionType);
  connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &TreeView::resizeAllColumns, connectionType);
}

void TreeView::unsetConnections() {
  disconnect(this, &QTreeView::expanded, this, &TreeView::resizeAllColumns);
  disconnect(this, &QTreeView::collapsed, this, &TreeView::resizeAllColumns);
  disconnect(this, &QWidget::customContextMenuRequested, this, &TreeView::showContextMenu);
  disconnect(verticalScrollBar(), &QScrollBar::valueChanged, this, &TreeView::resizeAllColumns);
}

void TreeView::showContextMenu(const QPoint &pos) {
  QMenu contextMenu;

  QAction actionExpand("Expandir tudo", this);
  connect(&actionExpand, &QAction::triggered, this, &TreeView::expandAll);
  contextMenu.addAction(&actionExpand);

  QAction actionCollapse("Recolher tudo", this);
  connect(&actionCollapse, &QAction::triggered, this, &TreeView::collapseAll);
  contextMenu.addAction(&actionCollapse);

  QAction action("Autodimensionar", this);
  action.setCheckable(true);
  action.setChecked(autoResize);
  connect(&action, &QAction::triggered, this, &TreeView::setAutoResize);
  contextMenu.addAction(&action);

  contextMenu.exec(mapToGlobal(pos));
}

void TreeView::collapseAll() {
  unsetConnections();

  QTreeView::collapseAll();
  resizeAllColumns();

  setConnections();
}

void TreeView::expandAll() {
  unsetConnections();

  QTreeView::expandAll();
  resizeAllColumns();

  setConnections();
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
  if (autoResize) {
    for (int col = 0; col < model()->columnCount(); ++col) { resizeColumnToContents(col); }
  }
}

int TreeView::columnIndex(const QString &column) const {
  int columnIndex = -1;

  if (baseModel) { columnIndex = baseModel->fieldIndex(column); }

  if (columnIndex == -1 and column != "created" and column != "lastUpdated") { qApp->enqueueError("Coluna '" + column + "' não encontrada!"); }

  return columnIndex;
}

void TreeView::setAutoResize(const bool value) { autoResize = value; }
