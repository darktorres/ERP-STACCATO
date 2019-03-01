#ifndef DATEFORMATDELEGATE_H
#define DATEFORMATDELEGATE_H

#include <QDate>
#include <QStyledItemDelegate>

class DateFormatDelegate final : public QStyledItemDelegate {

public:
  explicit DateFormatDelegate(const QDate defaultDate, QObject *parent = nullptr);
  ~DateFormatDelegate() = default;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &) const -> QWidget * final;

private:
  // attributes
  const QDate defaultDate;
  // methods
  auto displayText(const QVariant &value, const QLocale &) const -> QString final;
};

#endif // DATEFORMATDELEGATE_H
