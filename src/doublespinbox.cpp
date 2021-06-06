#include "doublespinbox.h"

#include <QDebug>

DoubleSpinBox::DoubleSpinBox(QWidget *parent) : QDoubleSpinBox(parent) {
  // mapper uses setProperty in place of setValue, update valorIntacto manually
  connect(this, qOverload<double>(&QDoubleSpinBox::valueChanged), [&](double value) { valorIntacto = value; });
}

void DoubleSpinBox::setValue(const double value) {
  valorIntacto = value;
  QDoubleSpinBox::setValue(value);
}

double DoubleSpinBox::value() const { return valorIntacto; }

double DoubleSpinBox::baseValue() const { return QDoubleSpinBox::value(); }
