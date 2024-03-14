#pragma once

#include <QSqlQueryModel>
#include <QTableView>

class TableView final : public QTableView {
  Q_OBJECT

public:
  explicit TableView(QWidget *parent);
  ~TableView() final = default;

  auto columnCount() const -> int;
  auto columnIndex(const QString &column) const -> int;
  auto columnIndex(const QString &column, const bool silent) const -> int;
  auto hideColumn(const QString &column) -> void;
  auto redoView() -> void;
  auto resort() -> void;
  auto rowCount() const -> int;
  auto setAutoResize(const bool value) -> void;
  auto setCopyHeaders(const bool newCopyHeaders) -> void;
  auto setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) -> void;
  auto setModel(QAbstractItemModel *model) -> void final;
  auto setPersistentColumns(const QStringList &value) -> void;
  auto setStoredSelection(bool newStoredSelection) -> void;
  auto showColumn(const QString &column) -> void;
  auto sortByColumn(const QString &column, Qt::SortOrder order = Qt::AscendingOrder) -> void;

protected:
  auto keyPressEvent(QKeyEvent *event) -> void final;
  auto mousePressEvent(QMouseEvent *event) -> void final;
  auto resizeEvent(QResizeEvent *event) -> void final;

private:
  // attributes
  bool autoResize = true;
  bool copyHeaders = true;
  bool storedSelection = false;
  QSqlQueryModel *baseModel = nullptr;
  QStringList persistentColumns;
  QVector<int> selectedRows;
  // methods
  auto openPersistentEditor(const int row, const QString &column) -> void;
  auto resizeColumnsToContents() -> void;
  auto restoreSelection() -> void;
  auto setConnections() -> void;
  auto showContextMenu(const QPoint pos) -> void;
  auto storeSelection() -> void;
};
