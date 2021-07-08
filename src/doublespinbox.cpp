#include "doublespinbox.h"

#include <QDebug>

DoubleSpinBox::DoubleSpinBox(QWidget *parent) : QDoubleSpinBox(parent) { connect(this, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &DoubleSpinBox::resizeToContent); }

void DoubleSpinBox::resizeToContent() {
  const int fmSize = fontMetrics().boundingRect(text()).width();
  const int buttonsSize = (buttonSymbols() == NoButtons ? 0 : 12);

  setFixedWidth(fmSize + buttonsSize + 20);
}

// TODO: adicionar lógica para mostrar 2 decimais mas guardar internamente 4 decimais
