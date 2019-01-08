#include <QAbstractProxyModel>
#include <QClipboard>
#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QSqlRecord>

#include "application.h"
#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"
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

void TableView::openPersistentEditor(const int row, const QString &column) { QTableView::openPersistentEditor(QTableView::model()->index(row, getColumnIndex(column))); }

void TableView::openPersistentEditor(const int row, const int column) { QTableView::openPersistentEditor(QTableView::model()->index(row, column)); }

void TableView::sortByColumn(const QString &column, Qt::SortOrder order) { QTableView::sortByColumn(getColumnIndex(column), order); }

void TableView::redoView() {
  if (blockRedo) { return; }

  if (not persistentColumns.isEmpty()) {
    for (int row = 0, rowCount = model()->rowCount(); row < rowCount; ++row) {
      for (const auto &column : persistentColumns) { openPersistentEditor(row, column); }
    }
  }

  if (autoResize) { resizeColumnsToContents(); }
}

void TableView::setModel(QAbstractItemModel *model) {
  QTableView::setModel(model);

  baseModel = qobject_cast<QSqlQueryModel *>(model);

  if (not baseModel) {
    auto proxyModel = qobject_cast<QAbstractProxyModel *>(QTableView::model());
    if (proxyModel) { baseModel = qobject_cast<QSqlQueryModel *>(proxyModel->sourceModel()); }
  }

  if (not baseModel) { return qApp->enqueueError("TableView model não implementado!", this); }

  //---------------------------------------

  connect(baseModel, &QSqlQueryModel::modelReset, this, &TableView::redoView);
  connect(baseModel, &QSqlQueryModel::dataChanged, this, &TableView::redoView);
  connect(baseModel, &QSqlQueryModel::rowsRemoved, this, &TableView::redoView);

  //---------------------------------------

  hideColumn("created");
  hideColumn("lastUpdated");
}

void TableView::enterEvent(QEvent *event) {
  if (autoResize) { resizeColumnsToContents(); }

  QTableView::enterEvent(event);
}

void TableView::mousePressEvent(QMouseEvent *event) {
  const QModelIndex item = indexAt(event->pos());

  if (not item.isValid()) { emit clicked(item); }

  QTableView::mousePressEvent(event);
}

void TableView::setBlockRedo(const bool value) {
  blockRedo = value;

  if (not blockRedo) { redoView(); }
}

void TableView::setPersistentColumns(const QStringList &value) { persistentColumns = value; }

void TableView::keyPressEvent(QKeyEvent *event) {
  if (event->matches(QKeySequence::Copy)) {
    QModelIndexList cells = selectedIndexes();
    std::sort(cells.begin(), cells.end()); // Necessary, otherwise they are in column order

    QString text;
    int currentRow = 0; // To determine when to insert newlines
    for (const QModelIndex &cell : cells) {
      if (text.length() == 0) {
        // First item
      } else if (cell.row() != currentRow) {
        // New row
        text += '\n';
      } else {
        // Next cell
        text += '\t';
      }

      currentRow = cell.row();
      text += cell.data().toString();
    }

    QApplication::clipboard()->setText(text);
  }

  QTableView::keyPressEvent(event);
}
