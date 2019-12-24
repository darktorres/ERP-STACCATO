#include "doubledelegate.h"
#include "usersession.h"

#include <QDoubleSpinBox>
#include <QPainter>

DoubleDelegate::DoubleDelegate(QObject *parent, const int decimais, const bool customPaint) : QStyledItemDelegate(parent), decimais(decimais), customPaint(customPaint) {}

QString DoubleDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  return (value.userType() == QVariant::Double) ? QLocale(QLocale::Portuguese).toString(value.toDouble(), 'f', decimais) : QStyledItemDelegate::displayText(value, locale);
}

QWidget *DoubleDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  Q_UNUSED(option)
  Q_UNUSED(index)

  auto *editor = new QDoubleSpinBox(parent);

  editor->setDecimals(decimais);

  return editor;
}

void DoubleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QStyledItemDelegate::paint(painter, option, index);

  if (not customPaint) { return; }

  // QTreeView doesn't paint grid, paint it manually

  const QString tema = UserSession::getSetting("User/tema").value_or("claro").toString();

  const QColor color = (tema == "escuro") ? QColor(44, 44, 44) : QColor(200, 200, 200);
  painter->setPen(color);
  painter->drawRect(option.rect);
}
