#pragma once

#include <QComboBox>

class ComboBox final : public QComboBox {
  Q_OBJECT

public:
  explicit ComboBox(QWidget *parent);
  ~ComboBox() = default;
  auto getCurrentValue() const -> QVariant;
  auto setCurrentValue(const QVariant &value) -> bool;

private:
  Q_PROPERTY(QVariant currentValue READ getCurrentValue WRITE setCurrentValue STORED false)
};
