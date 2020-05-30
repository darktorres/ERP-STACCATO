#pragma once

#include <QDate>
#include <QStyledItemDelegate>

class DateFormatDelegate final : public QStyledItemDelegate {

public:
  explicit DateFormatDelegate(const int vencimentoColumn, QObject *parent);
  explicit DateFormatDelegate(QObject *parent);
  ~DateFormatDelegate() = default;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;

private:
  // attributes
  const int vencimentoColumn = -1;
  // methods
  auto displayText(const QVariant &value, const QLocale &) const -> QString final;
};
