#pragma once

#include <QStyledItemDelegate>

class PorcentagemDelegate final : public QStyledItemDelegate {

public:
  explicit PorcentagemDelegate(QObject *parent = nullptr, const bool customPaint = false);
  ~PorcentagemDelegate() = default;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString;

private:
  const bool customPaint = false;
  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};
