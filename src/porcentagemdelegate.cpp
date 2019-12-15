#include "porcentagemdelegate.h"
#include "usersession.h"

#include <QPainter>

PorcentagemDelegate::PorcentagemDelegate(QObject *parent, const bool customPaint) : QStyledItemDelegate(parent), customPaint(customPaint) {}

QString PorcentagemDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  if (value.userType() == QVariant::Double or value.userType() == QVariant::Int) { return locale.toString(value.toDouble(), 'f', 2) + "%"; }

  return QStyledItemDelegate::displayText(value, locale);
}

void PorcentagemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QStyledItemDelegate::paint(painter, option, index);

  if (not customPaint) { return; }

  const QString tema = UserSession::getSetting("User/tema").value_or("claro").toString();

  const QColor color = (tema == "escuro") ? QColor(44, 44, 44) : QColor(200, 200, 200);
  painter->setPen(color);
  painter->drawRect(option.rect);
}
