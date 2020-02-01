#pragma once

#include <QLineEdit>

class LineEditDecimal final : public QLineEdit {
  Q_OBJECT

public:
  explicit LineEditDecimal(QWidget *parent = nullptr);
  auto getValue() const -> double;
  auto setValue(const double value) -> void;

private:
  const int decimais = 2;
};
