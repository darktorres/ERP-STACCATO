#ifndef DATEFORMATDELEGATE_H
#define DATEFORMATDELEGATE_H

#include <QDate>
#include <QStyledItemDelegate>

class DateFormatDelegate final : public QStyledItemDelegate {

public:
  explicit DateFormatDelegate(const QString defaultDateColumn, QObject *parent = nullptr);
  explicit DateFormatDelegate(QObject *parent = nullptr);
  ~DateFormatDelegate() = default;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;

private:
  // attributes
  const QString defaultDateColumn;
  // methods
  auto displayText(const QVariant &value, const QLocale &) const -> QString final;
};

#endif // DATEFORMATDELEGATE_H
