#pragma once

#include <QStyledItemDelegate>

class DateFormatDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit DateFormatDelegate(const int vencimentoColumn, QObject *parent);
  explicit DateFormatDelegate(QObject *parent);
  ~DateFormatDelegate() = default;

private:
  // attributes
  int const vencimentoColumn;
  // methods
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
};
