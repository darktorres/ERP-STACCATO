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
  auto openPersistentEditor(const int row, const QString &column) -> void;
  auto openPersistentEditor(const int row, const int column) -> void;
  auto resizeColumnsToContents() -> void;
  auto setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) -> void;
  auto setModel(QAbstractItemModel *model) -> void final;
  auto showColumn(const QString &column) -> void;
  auto sortByColumn(const QString &column, Qt::SortOrder order = Qt::AscendingOrder) -> void;

protected:
  virtual auto enterEvent(QEvent *event) -> void override;

private:
  // attributes
  bool autoResize = true;
  QSqlQueryModel *baseModel = nullptr;
  // methods
  auto getColumnIndex(const QString &column) -> int;
  auto showContextMenu(const QPoint &pos) -> void;
  auto toggleAutoResize() -> void;
};

#endif // TABLEVIEW_H
