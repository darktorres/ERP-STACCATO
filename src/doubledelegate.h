#pragma once

#include <QStyledItemDelegate>

class DoubleDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit DoubleDelegate(const int decimais, QObject *parent);
  explicit DoubleDelegate(QObject *parent);
  ~DoubleDelegate() final = default;

private:
  // attributes
  int const decimais = 2;
  // methods
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
};
