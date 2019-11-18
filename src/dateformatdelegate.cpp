#include <QDateEdit>

#include "application.h"
#include "dateformatdelegate.h"
#include "sqlrelationaltablemodel.h"

DateFormatDelegate::DateFormatDelegate(const int vencimentoColumn, QObject *parent) : QStyledItemDelegate(parent), vencimentoColumn(vencimentoColumn) {}

DateFormatDelegate::DateFormatDelegate(QObject *parent) { DateFormatDelegate(-1, parent); }

QString DateFormatDelegate::displayText(const QVariant &value, const QLocale &) const { return value.toDate().toString("dd/MM/yyyy"); }

QWidget *DateFormatDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const {
  auto *editor = new QDateEdit(parent);
  editor->setDate(qApp->serverDate());
  editor->setDisplayFormat("dd/MM/yy");
  editor->setCalendarPopup(true);

  if (vencimentoColumn != -1) { editor->setDate(index.siblingAtColumn(vencimentoColumn).data().toDate()); }

  return editor;
}
