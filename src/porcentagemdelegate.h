#pragma once

#include <QStyledItemDelegate>

class PorcentagemDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit PorcentagemDelegate(const bool multiplicar_, QObject *parent);
  ~PorcentagemDelegate() final = default;

private:
  // attributes
  bool const multiplicar;
  // methods
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString override;
};
