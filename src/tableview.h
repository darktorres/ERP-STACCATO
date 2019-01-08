#ifndef TABLEVIEW_H
#define TABLEVIEW_H

#include <QSqlQueryModel>
#include <QTableView>

class TableView final : public QTableView {
  Q_OBJECT

public:
  explicit TableView(QWidget *parent = nullptr);
  ~TableView() final = default;
  auto hideColumn(const QString &column) -> void;
  auto openPersistentEditor(const int row, const int column) -> void;
  auto setBlockRedo(const bool value) -> void;
  auto setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) -> void;
  auto setModel(QAbstractItemModel *model) -> void final;
  auto setPersistentColumns(const QStringList &value) -> void;
  auto showColumn(const QString &column) -> void;
  auto sortByColumn(const QString &column, Qt::SortOrder order = Qt::AscendingOrder) -> void;

protected:
  auto enterEvent(QEvent *event) -> void final;
  auto keyPressEvent(QKeyEvent *event) -> void final;
  auto mousePressEvent(QMouseEvent *event) -> void final;

private:
  // attributes
  bool blockRedo = false;
  bool autoResize = true;
  QSqlQueryModel *baseModel = nullptr;
  QStringList persistentColumns;
  // methods
  auto getColumnIndex(const QString &column) -> int;
  auto openPersistentEditor(const int row, const QString &column) -> void;
  auto redoView() -> void;
  auto showContextMenu(const QPoint &pos) -> void;
  auto toggleAutoResize() -> void;
};

#endif // TABLEVIEW_H
