#include "editdelegate.h"

EditDelegate::EditDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QString EditDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  return value.userType() == QMetaType::Double ? QLocale(QLocale::Portuguese).toString(value.toDouble(), 'f', 3) : QStyledItemDelegate::displayText(value, locale);
}
