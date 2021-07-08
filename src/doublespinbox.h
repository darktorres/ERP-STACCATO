#pragma once

#include <QDoubleSpinBox>

class DoubleSpinBox final : public QDoubleSpinBox {
  Q_OBJECT

public:
  explicit DoubleSpinBox(QWidget *parent);

  auto resizeToContent() -> void;
};
