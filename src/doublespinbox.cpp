#include "doublespinbox.h"

#include <QDebug>

DoubleSpinBox::DoubleSpinBox(QWidget *parent) : QDoubleSpinBox(parent) {}

void DoubleSpinBox::setValue(const double value) {
  fullValue = value;
  QDoubleSpinBox::setValue(value);

  emit fullValueChanged(fullValue);
}

double DoubleSpinBox::value() const { return fullValue; }

double DoubleSpinBox::visibleValue() const { return QDoubleSpinBox::value(); }
