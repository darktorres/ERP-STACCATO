#ifndef NOEDITDELEGATE_H
#define NOEDITDELEGATE_H

#include <QStyledItemDelegate>

class NoEditDelegate final : public QStyledItemDelegate {

public:
  explicit NoEditDelegate(QObject *parent = 0);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &) const final;
};

#endif // NOEDITDELEGATE_H
