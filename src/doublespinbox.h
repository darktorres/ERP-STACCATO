#pragma once

#include <QDoubleSpinBox>

class DoubleSpinBox final : public QDoubleSpinBox {
  Q_OBJECT

public:
  explicit DoubleSpinBox(QWidget *parent);

  auto setValue(const double value) -> void;
  auto value() const -> double;
  auto baseValue() const -> double;

private:
  double valorIntacto = 0;
};
