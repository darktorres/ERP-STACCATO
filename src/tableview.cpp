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
  connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &TableView::resizeColumnsToContents);

  verticalHeader()->setResizeContentsPrecision(0);
  horizontalHeader()->setResizeContentsPrecision(0);

  verticalHeader()->setDefaultSectionSize(20);

  horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void TableView::resizeColumnsToContents() {
  if (autoResize) { QTableView::resizeColumnsToContents(); }
}

int TableView::columnCount() const { return model()->columnCount(); }

void TableView::showContextMenu(const QPoint &pos) {
  QMenu contextMenu;

  QAction action("Autodimensionar", this);
  action.setCheckable(true);
  action.setChecked(autoResize);
  connect(&action, &QAction::triggered, this, &TableView::setAutoResize);
  contextMenu.addAction(&action);

  contextMenu.exec(mapToGlobal(pos));
}

int TableView::columnIndex(const QString &column, const bool silent) const {
  int columnIndex = -1;

  if (baseModel) { columnIndex = baseModel->record().indexOf(column); }

  if (columnIndex == -1 and not silent and column != "created" and column != "lastUpdated") { qApp->enqueueError("Coluna '" + column + "' não encontrada!"); }

  return columnIndex;
}

void TableView::hideColumn(const QString &column) { QTableView::hideColumn(columnIndex(column)); }

void TableView::showColumn(const QString &column) { QTableView::showColumn(columnIndex(column)); }

void TableView::setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) { QTableView::setItemDelegateForColumn(columnIndex(column), delegate); }

void TableView::openPersistentEditor(const int row, const QString &column) { QTableView::openPersistentEditor(QTableView::model()->index(row, columnIndex(column))); }

void TableView::openPersistentEditor(const int row, const int column) { QTableView::openPersistentEditor(QTableView::model()->index(row, column)); }

void TableView::sortByColumn(const QString &column, Qt::SortOrder order) { QTableView::sortByColumn(columnIndex(column), order); }

int TableView::rowCount() const { return model()->rowCount(); }

void TableView::redoView() {
  if (persistentColumns.isEmpty()) { return; }

  for (int row = 0, rowCount = model()->rowCount(); row < rowCount; ++row) {
    for (const auto &column : persistentColumns) { openPersistentEditor(row, column); }
  }
}

void TableView::setModel(QAbstractItemModel *model) {
  if (auto temp = qobject_cast<SqlQueryModel *>(model); temp and temp->proxyModel) {
    QTableView::setModel(temp->proxyModel);
  } else if (auto temp2 = qobject_cast<SqlRelationalTableModel *>(model); temp2 and temp2->proxyModel) {
    QTableView::setModel(temp2->proxyModel);
  } else {
    QTableView::setModel(model);
  }

  baseModel = qobject_cast<QSqlQueryModel *>(model);

  if (not baseModel) { return qApp->enqueueError("TableView model não implementado!", this); }

  //---------------------------------------

  connect(baseModel, &QSqlQueryModel::modelReset, this, &TableView::redoView);
  connect(baseModel, &QSqlQueryModel::dataChanged, this, &TableView::redoView);
  connect(baseModel, &QSqlQueryModel::rowsRemoved, this, &TableView::redoView);

  //---------------------------------------

  hideColumn("created");
  hideColumn("lastUpdated");

  redoView();
}

void TableView::mousePressEvent(QMouseEvent *event) {
  const QModelIndex item = indexAt(event->pos());

  if (not item.isValid()) {
    clearSelection();
    // QTableView don't emit when index is invalid, emit manually for widgets
    emit clicked(item);
  }

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
