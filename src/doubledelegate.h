#pragma once

#include <QStyledItemDelegate>

class DoubleDelegate final : public QStyledItemDelegate {

public:
  explicit DoubleDelegate(QObject *parent = nullptr, const int decimais = 2);
  ~DoubleDelegate() = default;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;

private:
  const int decimais;
};
