#pragma once

#include <QTreeView>

#include "sqltreemodel.h"

class TreeView final : public QTreeView {
  Q_OBJECT

public:
  explicit TreeView(QWidget *parent = nullptr);
  ~TreeView() final = default;
  auto hideColumn(const QString &column) -> void;
  auto setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) -> void;
  auto setModel(QAbstractItemModel *model) -> void;

private:
  // attributes
  SqlTreeModel *baseModel = nullptr;
  // methods
  auto columnIndex(const QString &column) const -> int;
  auto resizeAllColumns() -> void;
};
