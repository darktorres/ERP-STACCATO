#include <QDateTimeEdit>

#include "dateformatdelegate.h"
#include "sqlrelationaltablemodel.h"

DateFormatDelegate::DateFormatDelegate(const QString defaultDateColumn, QObject *parent) : QStyledItemDelegate(parent), defaultDateColumn(defaultDateColumn) {}

DateFormatDelegate::DateFormatDelegate(QObject *parent) { DateFormatDelegate("", parent); }

QString DateFormatDelegate::displayText(const QVariant &value, const QLocale &) const { return value.toDate().toString("dd/MM/yyyy"); }

QWidget *DateFormatDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const {
  auto *editor = new QDateTimeEdit(parent);
  editor->setDate(QDate::currentDate());
  editor->setDisplayFormat("dd/MM/yy");
  editor->setCalendarPopup(true);

  auto model = qobject_cast<SqlRelationalTableModel *>(const_cast<QAbstractItemModel *>(index.model()));

  if (model and not defaultDateColumn.isEmpty()) { editor->setDate(index.siblingAtColumn(model->fieldIndex(defaultDateColumn)).data().toDate()); }

  return editor;
}
