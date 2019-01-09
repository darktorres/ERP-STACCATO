#include "doubledelegate.h"

#include <QDoubleSpinBox>

DoubleDelegate::DoubleDelegate(QObject *parent, const int decimais) : QStyledItemDelegate(parent), decimais(decimais) {}

QString DoubleDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  return value.userType() == QVariant::Double ? QLocale(QLocale::Portuguese).toString(value.toDouble(), 'f', decimais) : QStyledItemDelegate::displayText(value, locale);
}

QWidget *DoubleDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  Q_UNUSED(option);
  Q_UNUSED(index);

  auto *editor = new QDoubleSpinBox(parent);

  editor->setDecimals(decimais);

  return editor;
}
