#pragma once

#include <QStyledItemDelegate>

class ReaisDelegate final : public QStyledItemDelegate {

public:
  explicit ReaisDelegate(const int decimais, const bool readOnly, QObject *parent);
  explicit ReaisDelegate(QObject *parent);
  ~ReaisDelegate() = default;

  auto displayText(const QVariant &value, const QLocale &locale) const -> QString override;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * override;

private:
  bool const readOnly = false;
  int const decimais = 2;
};
