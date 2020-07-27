#include "tableview.h"

#include "application.h"
#include "sqlquerymodel.h"
#include "sqltablemodel.h"

#include <QClipboard>
#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>
#include <QSqlRecord>

TableView::TableView(QWidget *parent) : QTableView(parent) {
  setContextMenuPolicy(Qt::CustomContextMenu);

  verticalHeader()->setResizeContentsPrecision(0);
  horizontalHeader()->setResizeContentsPrecision(0);

  verticalHeader()->setDefaultSectionSize(20);
  horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  connect(this, &QWidget::customContextMenuRequested, this, &TableView::showContextMenu);
  connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &TableView::resizeColumnsToContents);
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

void TableView::resizeEvent(QResizeEvent *event) {
  redoView();

  QTableView::resizeEvent(event);
}

int TableView::columnIndex(const QString &column) const { return columnIndex(column, false); }

int TableView::columnIndex(const QString &column, const bool silent) const {
  int columnIndex = -1;

  if (baseModel) { columnIndex = baseModel->record().indexOf(column); }

  if (columnIndex == -1 and not silent and column != "created" and column != "lastUpdated") { qApp->enqueueException("Coluna '" + column + "' não encontrada!"); }

  return columnIndex;
}

void TableView::hideColumn(const QString &column) { QTableView::hideColumn(columnIndex(column)); }

void TableView::showColumn(const QString &column) { QTableView::showColumn(columnIndex(column)); }

void TableView::setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) { QTableView::setItemDelegateForColumn(columnIndex(column), delegate); }

void TableView::openPersistentEditor(const int row, const QString &column) { QTableView::openPersistentEditor(QTableView::model()->index(row, columnIndex(column))); }

void TableView::openPersistentEditor(const int row, const int column) { QTableView::openPersistentEditor(QTableView::model()->index(row, column)); }

void TableView::resort() { model()->sort(horizontalHeader()->sortIndicatorSection(), horizontalHeader()->sortIndicatorOrder()); }

void TableView::sortByColumn(const QString &column, Qt::SortOrder order) { QTableView::sortByColumn(columnIndex(column), order); }

int TableView::rowCount() const { return model()->rowCount(); }

void TableView::redoView() {
  if (persistentColumns.isEmpty()) { return; }
  if (rowCount() == 0) { return; }

  const int firstRowIndex = indexAt(QPoint(viewport()->rect().x() + 5, viewport()->rect().y() + 5)).row();
  int lastRowIndex = indexAt(QPoint(viewport()->rect().x() + 5, viewport()->rect().height() - 5)).row();

  if (firstRowIndex == -1) { return; }

  int count = 0;

  // subtrair uma linha de altura por vez até achar uma linha
  while (lastRowIndex == -1) {
    int xpos = viewport()->rect().x() + 5;
    int ypos = viewport()->rect().height() - 5 - rowHeight(0) * count;

    if (ypos < 0) { return; }

    lastRowIndex = indexAt(QPoint(xpos, ypos)).row();
    ++count;
  }

  for (int row = firstRowIndex; row <= lastRowIndex; ++row) {
    for (const auto &column : persistentColumns) { openPersistentEditor(row, column); }
  }
}

void TableView::setModel(QAbstractItemModel *model) {
  if (auto queryModel = qobject_cast<SqlQueryModel *>(model); queryModel and queryModel->proxyModel) {
    QTableView::setModel(queryModel->proxyModel);
  } else if (auto tableModel = qobject_cast<SqlTableModel *>(model); tableModel and tableModel->proxyModel) {
    QTableView::setModel(tableModel->proxyModel);
  } else {
    QTableView::setModel(model);
  }

  baseModel = qobject_cast<QSqlQueryModel *>(model);

  if (not baseModel) { return qApp->enqueueException("TableView model não implementado!", this); }

  //---------------------------------------

  connect(baseModel, &QSqlQueryModel::modelReset, this, &TableView::redoView);
  connect(baseModel, &QSqlQueryModel::dataChanged, this, &TableView::redoView);
  connect(baseModel, &QSqlQueryModel::rowsRemoved, this, &TableView::redoView);
  connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &TableView::redoView);

  //---------------------------------------

  hideColumn("created");
  hideColumn("lastUpdated");

  redoView();
}

void TableView::mousePressEvent(QMouseEvent *event) {
  const QModelIndex item = indexAt(event->pos());

  if (not item.isValid()) {
    clearSelection();

    emit clicked(item); // QTableView don't emit when index is invalid, emit manually for widgets
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
