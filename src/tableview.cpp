#include <QAbstractProxyModel>
#include <QClipboard>
#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>
#include <QSqlRecord>

#include "application.h"
#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"
#include "tableview.h"

TableView::TableView(QWidget *parent) : QTableView(parent) {
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &QWidget::customContextMenuRequested, this, &TableView::showContextMenu);

  connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, [&] {
    if (autoResize) { resizeColumnsToContents(); }
  });

  verticalHeader()->setResizeContentsPrecision(0);
  horizontalHeader()->setResizeContentsPrecision(0);

  verticalHeader()->setDefaultSectionSize(20);

  horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void TableView::showContextMenu(const QPoint &pos) {
  QMenu contextMenu;

  QAction action("Autodimensionar", this);
  action.setCheckable(true);
  action.setChecked(autoResize);
  connect(&action, &QAction::triggered, this, &TableView::setAutoResize);
  contextMenu.addAction(&action);

  contextMenu.exec(mapToGlobal(pos));
}

int TableView::getColumnIndex(const QString &column) const {
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
  if (not persistentColumns.isEmpty()) {
    for (int row = 0, rowCount = model()->rowCount(); row < rowCount; ++row) {
      for (const auto &column : persistentColumns) { openPersistentEditor(row, column); }
    }
  }
}

void TableView::setModel(QAbstractItemModel *model) {
  if (auto sqlQuery = qobject_cast<SqlQueryModel *>(model); sqlQuery and sqlQuery->proxyModel) {
    QTableView::setModel(sqlQuery->proxyModel);
  } else if (auto sqlRelat = qobject_cast<SqlRelationalTableModel *>(model); sqlRelat and sqlRelat->proxyModel) {
    QTableView::setModel(sqlRelat->proxyModel);
  } else {
    QTableView::setModel(model);
  }

  baseModel = qobject_cast<QSqlQueryModel *>(model);

  //---------------------------------------

  if (baseModel) {
    connect(baseModel, &QSqlQueryModel::modelReset, this, &TableView::redoView);
    connect(baseModel, &QSqlQueryModel::dataChanged, this, &TableView::redoView);
    connect(baseModel, &QSqlQueryModel::rowsRemoved, this, &TableView::redoView);
  }

  //---------------------------------------

  hideColumn("created");
  hideColumn("lastUpdated");

  redoView();
}

void TableView::mousePressEvent(QMouseEvent *event) {
  const QModelIndex item = indexAt(event->pos());

  // this enables clicking outside of lines to clear selection
  if (not item.isValid()) { emit clicked(item); }

  QTableView::mousePressEvent(event);
}

void TableView::setAutoResize(const bool value) {
  autoResize = value;

  horizontalHeader()->setSectionResizeMode(autoResize ? QHeaderView::ResizeToContents : QHeaderView::Interactive);
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
