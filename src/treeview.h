#pragma once

#include "sqltreemodel.h"

#include <QStack>
#include <QTreeView>

class TreeView final : public QTreeView {
  Q_OBJECT

public:
  explicit TreeView(QWidget *parent);
  ~TreeView() final = default;

  auto hideColumn(const QString &column) -> void;
  auto setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) -> void;
  auto setModel(QAbstractItemModel *model) -> void final;

protected:
  auto drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const -> void final;
  auto mousePressEvent(QMouseEvent *event) -> void final;
  auto keyPressEvent(QKeyEvent *event) -> void final;

private:
  // attributes
  bool autoResize = true;
  QStack<int> blockingSignals;
  SqlTreeModel *baseModel = nullptr;
  // methods
  auto collapseAll() -> void;
  auto columnIndex(const QString &column) const -> int;
  auto expandAll() -> void;
  auto resizeAllColumns() -> void;
  auto setAutoResize(const bool value) -> void;
  auto setConnections() -> void;
  auto showContextMenu(const QPoint pos) -> void;
  auto unsetConnections() -> void;
};
