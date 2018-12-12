#include <QAbstractProxyModel>
#include <QHeaderView>
#include <QMenu>
#include <QSqlRecord>

#include "application.h"
#include "tableview.h"

TableView::TableView(QWidget *parent) : QTableView(parent) {
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &QWidget::customContextMenuRequested, this, &TableView::showContextMenu);

  verticalHeader()->setResizeContentsPrecision(0);
  horizontalHeader()->setResizeContentsPrecision(0);

  verticalHeader()->setDefaultSectionSize(20);

  setMouseTracking(true);
}

void TableView::showContextMenu(const QPoint &pos) {
  QMenu contextMenu;

  QAction action("Autodimensionar", this);
  action.setCheckable(true);
  action.setChecked(autoResize);
  connect(&action, &QAction::triggered, this, &TableView::toggleAutoResize);
  contextMenu.addAction(&action);

  contextMenu.exec(mapToGlobal(pos));
}

void TableView::toggleAutoResize() { autoResize = not autoResize; }

int TableView::getColumnIndex(const QString &column) {
  int columnIndex = -1;

  if (baseModel) { columnIndex = baseModel->record().indexOf(column); }

  return columnIndex;
}

void TableView::hideColumn(const QString &column) { QTableView::hideColumn(getColumnIndex(column)); }

void TableView::showColumn(const QString &column) { QTableView::showColumn(getColumnIndex(column)); }

void TableView::setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) { QTableView::setItemDelegateForColumn(getColumnIndex(column), delegate); }

void TableView::openPersistentEditor(const int row, const QString &column) { QTableView::openPersistentEditor(baseModel->index(row, getColumnIndex(column))); }

void TableView::openPersistentEditor(const int row, const int column) { QTableView::openPersistentEditor(QTableView::model()->index(row, column)); }

void TableView::sortByColumn(const QString &column, Qt::SortOrder order) { QTableView::sortByColumn(getColumnIndex(column), order); }

void TableView::resizeColumnsToContents() {
  if (autoResize) { QTableView::resizeColumnsToContents(); }
}

void TableView::setModel(QAbstractItemModel *model) {
  QTableView::setModel(model);

  baseModel = qobject_cast<QSqlQueryModel *>(model);

  if (not baseModel) {
    auto proxyModel = qobject_cast<QAbstractProxyModel *>(QTableView::model());
    if (proxyModel) { baseModel = qobject_cast<QSqlQueryModel *>(proxyModel->sourceModel()); }
  }

  if (not baseModel) { qApp->enqueueError("TableView model não implementado!"); }

  //---------------------------------------

  hideColumn("created");
  hideColumn("lastUpdated");
}

void TableView::enterEvent(QEvent *event) {
  if (autoResize) { resizeColumnsToContents(); }
  QTableView::enterEvent(event);
}

// TODO: 4program copy - http://stackoverflow.com/questions/3135737/copying-part-of-qtableview
