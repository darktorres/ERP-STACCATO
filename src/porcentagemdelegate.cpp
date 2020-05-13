#include "porcentagemdelegate.h"

PorcentagemDelegate::PorcentagemDelegate(const bool multiplicar, QObject *parent) : QStyledItemDelegate(parent), multiplicar(multiplicar) {}

QString PorcentagemDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  if (value.userType() == QVariant::Double or value.userType() == QVariant::Int) {
    const double newValue = multiplicar ? value.toDouble() * 100 : value.toDouble();
    return locale.toString(newValue, 'f', 2) + "%";
  }

  return QStyledItemDelegate::displayText(value, locale);
}
