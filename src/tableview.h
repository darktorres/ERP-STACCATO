#pragma once

#include <QSqlQueryModel>
#include <QTableView>

class TableView final : public QTableView {
  Q_OBJECT

public:
  explicit TableView(QWidget *parent = nullptr);
  ~TableView() final = default;
  auto columnCount() const -> int;
  auto columnIndex(const QString &column, const bool silent = false) const -> int;
  auto hideColumn(const QString &column) -> void;
  auto openPersistentEditor(const int row, const int column) -> void;
  auto resort() -> void;
  auto rowCount() const -> int;
  auto setAutoResize(const bool value) -> void;
  auto setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) -> void;
  auto setModel(QAbstractItemModel *model) -> void;
  auto setPersistentColumns(const QStringList &value) -> void;
  auto showColumn(const QString &column) -> void;
  auto sortByColumn(const QString &column, Qt::SortOrder order = Qt::AscendingOrder) -> void;

protected:
  auto keyPressEvent(QKeyEvent *event) -> void final;
  auto mousePressEvent(QMouseEvent *event) -> void final;
  auto resizeEvent(QResizeEvent *event) -> void final;

private:
  // attributes
  bool autoResize = true;
  QSqlQueryModel *baseModel = nullptr;
  QStringList persistentColumns;
  // methods
  auto openPersistentEditor(const int row, const QString &column) -> void;
  auto redoView() -> void;
  auto resizeColumnsToContents() -> void;
  auto showContextMenu(const QPoint &pos) -> void;
};
