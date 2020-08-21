#pragma once

#include <QStyledItemDelegate>

class DoubleDelegate final : public QStyledItemDelegate {

public:
  explicit DoubleDelegate(const int decimais, QObject *parent);
  explicit DoubleDelegate(QObject *parent);
  ~DoubleDelegate() = default;

private:
  // attributes
  const int decimais = 2;
  // methods
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
};
