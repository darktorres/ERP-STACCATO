#ifndef SINGLEEDITDELEGATE_H
#define SINGLEEDITDELEGATE_H

#include <QStyledItemDelegate>

// TODO: rename this to EditDelegate?
class SingleEditDelegate final : public QStyledItemDelegate {

public:
  explicit SingleEditDelegate(QObject *parent = nullptr);
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
};

#endif // SINGLEEDITDELEGATE_H
