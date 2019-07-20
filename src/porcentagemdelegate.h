#pragma once

#include <QStyledItemDelegate>

class PorcentagemDelegate final : public QStyledItemDelegate {

public:
  explicit PorcentagemDelegate(QObject *parent = nullptr);
  ~PorcentagemDelegate() = default;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString;
};
