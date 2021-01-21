#include "reaisdelegate.h"

ReaisDelegate::ReaisDelegate(const int decimais, const bool readOnly, QObject *parent) : QStyledItemDelegate(parent), readOnly(readOnly), decimais(decimais) {}

ReaisDelegate::ReaisDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QString ReaisDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  return value.userType() == QVariant::Double ? "R$ " + QLocale(QLocale::Portuguese).toString(value.toDouble(), 'f', decimais) : QStyledItemDelegate::displayText(value, locale);
}

QWidget *ReaisDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  if (readOnly) { return nullptr; }

  return QStyledItemDelegate::createEditor(parent, option, index);
}
