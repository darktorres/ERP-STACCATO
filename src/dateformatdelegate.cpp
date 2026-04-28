#include "dateformatdelegate.h"

#include "application.h"

#include <QDateEdit>

DateFormatDelegate::DateFormatDelegate(const int vencimentoColumn, QObject *parent)
    : QStyledItemDelegate(parent), vencimentoColumn(vencimentoColumn) {}

DateFormatDelegate::DateFormatDelegate(QObject *parent) : DateFormatDelegate(-1, parent) {}

QString DateFormatDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  Q_UNUSED(locale)

  return value.toDate().toString("dd/MM/yyyy");
}

QWidget *DateFormatDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  Q_UNUSED(option)

  auto *editor = new QDateEdit(parent);
  editor->setDate(qApp->serverDate());
  editor->setDisplayFormat("dd/MM/yy");
  editor->setCalendarPopup(true);

  if (vencimentoColumn != -1) {
    editor->setDate(qApp->ajustarDiaUtil(index.siblingAtColumn(vencimentoColumn).data().toDate()));
  }

  return editor;
}
