#include "porcentagemdelegate.h"

PorcentagemDelegate::PorcentagemDelegate(const bool multiplicar_, QObject *parent) : QStyledItemDelegate(parent), multiplicar(multiplicar_) {}

QString PorcentagemDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  if (value.userType() == QMetaType::Double or value.userType() == QMetaType::Int) {
    const double newValue = multiplicar ? value.toDouble() * 100 : value.toDouble();
    return locale.toString(newValue, 'f', 2) + "%";
  }

  return QStyledItemDelegate::displayText(value, locale);
}
