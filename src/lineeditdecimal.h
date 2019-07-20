#pragma once

#include <QLineEdit>

class LineEditDecimal final : public QLineEdit {
  Q_OBJECT

public:
  explicit LineEditDecimal(QWidget *parent = nullptr);
  auto getValue() const -> double;
  auto setValue(const double value) -> void;

private:
  // attributes
  int decimais = 2;
  // methods
};
