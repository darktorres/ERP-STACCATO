#include <QClipboard>
#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>
#include <QSqlRecord>

#include "application.h"
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
  if (not persistentColumns.isEmpty()) {
    for (int row = 0, rowCount = model()->rowCount(); row < rowCount; ++row) {
      for (const auto &column : persistentColumns) { openPersistentEditor(row, column); }
    }
  }
}

void TableView::setModel(QIdentityProxyModel *model) {
  baseModel = static_cast<QSqlQueryModel *>(model->sourceModel());

  setModel(static_cast<QAbstractItemModel *>(model));
}

void TableView::setModel(QSortFilterProxyModel *model) {
  baseModel = static_cast<QSqlQueryModel *>(model->sourceModel());

  setModel(static_cast<QAbstractItemModel *>(model));
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
void TableView::setModel(QTransposeProxyModel *model) {
  baseModel = static_cast<QSqlQueryModel *>(model->sourceModel());

  setModel(static_cast<QAbstractItemModel *>(model));
}
#endif

void TableView::setModel(QSqlQueryModel *model) {
  baseModel = model;

  setModel(static_cast<QAbstractItemModel *>(model));
}

void TableView::setModel(QAbstractItemModel *model) {
  QTableView::setModel(model);

  //---------------------------------------

  if (not baseModel) { qApp->enqueueError("Sem baseModel!"); }

  connect(model, &QAbstractItemModel::modelReset, this, &TableView::redoView);
  connect(model, &QAbstractItemModel::dataChanged, this, &TableView::redoView);
  connect(model, &QAbstractItemModel::rowsRemoved, this, &TableView::redoView);

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
