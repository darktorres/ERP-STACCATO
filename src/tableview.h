#pragma once

#include <QIdentityProxyModel>
#include <QSortFilterProxyModel>
#include <QSqlQueryModel>
#include <QTableView>

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
#include <QTransposeProxyModel>
#endif

class TableView final : public QTableView {
  Q_OBJECT

public:
  explicit TableView(QWidget *parent = nullptr);
  ~TableView() final = default;
  auto columnCount() const -> int;
  auto dataAt(const QModelIndex &index, const QString &column) const -> QVariant;
  auto dataAt(const int row, const QString &column) const -> QVariant;
  auto dataAt(const int row, const int column) const -> QVariant;
  auto columnIndex(const QString &column, const bool silent = false) const -> int;
  auto hideColumn(const QString &column) -> void;
  auto openPersistentEditor(const int row, const int column) -> void;
  auto rowCount() const -> int;
  auto setAutoResize(const bool value) -> void;
  auto setDataAt(const QModelIndex &index, const QString &column, const QVariant &value) -> bool;
  auto setDataAt(const int row, const QString &column, const QVariant &value) -> bool;
  auto setDataAt(const int row, const int column, const QVariant &value) -> bool;
  auto setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) -> void;
  auto setModel(QIdentityProxyModel *model) -> void;
  auto setModel(QSortFilterProxyModel *model) -> void;
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
  auto setModel(QTransposeProxyModel *model) -> void;
#endif
  auto setModel(QSqlQueryModel *model) -> void;
  auto setPersistentColumns(const QStringList &value) -> void;
  auto showColumn(const QString &column) -> void;
  auto sortByColumn(const QString &column, Qt::SortOrder order = Qt::AscendingOrder) -> void;

protected:
  auto keyPressEvent(QKeyEvent *event) -> void final;
  auto mousePressEvent(QMouseEvent *event) -> void final;

private:
  // attributes
  bool autoResize = true;
  QSqlQueryModel *baseModel = nullptr;
  QStringList persistentColumns;
  // methods
  auto openPersistentEditor(const int row, const QString &column) -> void;
  auto redoView() -> void;
  auto showContextMenu(const QPoint &pos) -> void;
  auto setModel(QAbstractItemModel *model) -> void;
};
