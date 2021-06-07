#pragma once

#include <QDoubleSpinBox>

class DoubleSpinBox final : public QDoubleSpinBox {
  Q_OBJECT

  // property for mapper to work correctly
  Q_PROPERTY(double fullValue READ value WRITE setValue NOTIFY fullValueChanged USER true)

public:
  explicit DoubleSpinBox(QWidget *parent);

  auto setValue(const double value) -> void;
  auto value() const -> double;
  auto visibleValue() const -> double;

signals:
  void fullValueChanged(double);

private:
  double fullValue = 0;
};
