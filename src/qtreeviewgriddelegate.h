#pragma once

#include <QStyledItemDelegate>

class QTreeViewGridDelegate : public QStyledItemDelegate {
public:
  explicit QTreeViewGridDelegate(QObject *parent = nullptr);

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};
