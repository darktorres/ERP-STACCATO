#ifndef SINGLEEDITDELEGATE_H
#define SINGLEEDITDELEGATE_H

#include <QStyledItemDelegate>

class SingleEditDelegate final : public QStyledItemDelegate {

public:
  explicit SingleEditDelegate(QObject *parent = 0);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const final;
  QString displayText(const QVariant &value, const QLocale &locale) const final;
};

#endif // SINGLEEDITDELEGATE_H
