#ifndef DATEFORMATDELEGATE_H
#define DATEFORMATDELEGATE_H

#include <QDate>
#include <QStyledItemDelegate>

class DateFormatDelegate final : public QStyledItemDelegate {

public:
  explicit DateFormatDelegate(QObject *parent = nullptr);
  ~DateFormatDelegate() = default;
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &) const final;

private:
  QString displayText(const QVariant &value, const QLocale &) const final;
};

#endif // DATEFORMATDELEGATE_H
