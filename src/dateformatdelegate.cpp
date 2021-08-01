#include "dateformatdelegate.h"

#include "application.h"

#include <QDateEdit>

DateFormatDelegate::DateFormatDelegate(const int vencimentoColumn, const int tipoColumn, const bool recebimento, QObject *parent)
    : QStyledItemDelegate(parent), recebimento(recebimento), tipoColumn(tipoColumn), vencimentoColumn(vencimentoColumn) {}

DateFormatDelegate::DateFormatDelegate(QObject *parent) : DateFormatDelegate(-1, -1, false, parent) {}

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

  if (vencimentoColumn != -1 and tipoColumn != -1) {
    const QString tipo = index.siblingAtColumn(tipoColumn).data().toString();
    const int dias = (tipo.contains("BOLETO") and recebimento) ? 1 : 0;

    editor->setDate(qApp->ajustarDiaUtil(index.siblingAtColumn(vencimentoColumn).data().toDate().addDays(dias)));
  }

  return editor;
}
