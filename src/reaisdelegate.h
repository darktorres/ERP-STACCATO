#pragma once

#include <QStyledItemDelegate>

class ReaisDelegate final : public QStyledItemDelegate {

public:
  explicit ReaisDelegate(QObject *parent = nullptr, const int decimais = 2, const bool customPaint = false);
  ~ReaisDelegate() = default;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString;

private:
  const int decimais;
  const bool customPaint = false;
  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};
