#include <QDateTimeEdit>

#include "dateformatdelegate.h"

DateFormatDelegate::DateFormatDelegate(const QDate defaultDate, QObject *parent) : QStyledItemDelegate(parent), defaultDate(defaultDate) {}

QString DateFormatDelegate::displayText(const QVariant &value, const QLocale &) const { return value.toDate().toString("dd/MM/yyyy"); }

QWidget *DateFormatDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const {
  auto *editor = new QDateTimeEdit(parent);
  editor->setDate(defaultDate);
  editor->setDisplayFormat("dd/MM/yy");
  editor->setCalendarPopup(true);

  return editor;
}
