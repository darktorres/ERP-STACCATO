#pragma once

#include <QLineEdit>

class LineEditDecimal final : public QLineEdit {
  Q_OBJECT

public:
  explicit LineEditDecimal(QWidget *parent);

  auto getValue() const -> double;
  auto setValue(const double value) -> void;

private:
  int const decimais = 2;
};
