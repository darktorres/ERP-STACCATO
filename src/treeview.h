#pragma once

#include "sqltreemodel.h"

#include <QIdentityProxyModel>
#include <QSortFilterProxyModel>
#include <QTreeView>

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
#include <QTransposeProxyModel>
#endif

class TreeView final : public QTreeView {
  Q_OBJECT

public:
  explicit TreeView(QWidget *parent = nullptr);
  ~TreeView() final = default;
  auto hideColumn(const QString &column) -> void;
  auto setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) -> void;
  auto setModel(QIdentityProxyModel *model) -> void;
  auto setModel(QSortFilterProxyModel *model) -> void;
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
  auto setModel(QTransposeProxyModel *model) -> void;
#endif
  auto setModel(SqlTreeModel *model) -> void;

private:
  // attributes
  SqlTreeModel *baseModel = nullptr;
  // methods
  auto columnIndex(const QString &column) const -> int;
  auto setModel(QAbstractItemModel *model) -> void;
  auto resizeAllColumns() -> void;
};
