#pragma once

#include "sqltreemodel.h"

#include <QTreeView>

class TreeView final : public QTreeView {
  Q_OBJECT

public:
  explicit TreeView(QWidget *parent = nullptr);
  ~TreeView() final = default;
  auto hideColumn(const QString &column) -> void;
  auto setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) -> void;
  auto setModel(QAbstractItemModel *model) -> void;

protected:
  auto mousePressEvent(QMouseEvent *event) -> void final;

private:
  // attributes
  bool autoResize = true;
  SqlTreeModel *baseModel = nullptr;
  // methods
  auto collapseAll() -> void;
  auto columnIndex(const QString &column) const -> int;
  auto expandAll() -> void;
  auto resizeAllColumns() -> void;
  auto setAutoResize(const bool value) -> void;
  auto setConnections() -> void;
  auto showContextMenu(const QPoint &pos) -> void;
  auto unsetConnections() -> void;
};
