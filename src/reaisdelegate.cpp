#include "reaisdelegate.h"

ReaisDelegate::ReaisDelegate(QObject *parent, const int decimais) : QStyledItemDelegate(parent), decimais(decimais) {}

QString ReaisDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  return value.userType() == QVariant::Double ? "R$ " + QLocale(QLocale::Portuguese).toString(value.toDouble(), 'f', decimais) : QStyledItemDelegate::displayText(value, locale);
}
