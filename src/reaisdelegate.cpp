#include "reaisdelegate.h"

#include "usersession.h"

#include <QPainter>

ReaisDelegate::ReaisDelegate(QObject *parent, const int decimais, const bool customPaint) : QStyledItemDelegate(parent), decimais(decimais), customPaint(customPaint) {}

QString ReaisDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  return value.userType() == QVariant::Double ? "R$ " + QLocale(QLocale::Portuguese).toString(value.toDouble(), 'f', decimais) : QStyledItemDelegate::displayText(value, locale);
}

void ReaisDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QStyledItemDelegate::paint(painter, option, index);

  if (not customPaint) { return; }

  const QString tema = UserSession::getSetting("User/tema").value_or("claro").toString();

  const QColor color = (tema == "escuro") ? QColor(44, 44, 44) : QColor(200, 200, 200);
  painter->setPen(color);
  painter->drawRect(option.rect);
}

// TODO: 4add a parameter for enabling/disabling editing (also in others delegate so as to remove NoEditDelegate)
