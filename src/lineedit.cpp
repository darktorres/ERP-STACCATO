#include "lineedit.h"

#include <QDebug>

LineEdit::LineEdit(QWidget *parent) : QLineEdit(parent) { connect(this, &QLineEdit::textChanged, this, &LineEdit::resizeToContent); }

void LineEdit::resizeToContent() {
  if (text().isEmpty()) { return setFixedWidth(baseSize().width()); }

  const int fmSize = fontMetrics().boundingRect(text()).width();

  setFixedWidth(qMax(fmSize + 15, baseSize().width()));
}

// TODO: alterar para que texto grande seja sempre cortado pela direita e não pela esquerda, ex:
// errado: "RRO AZUL"
// certo:  "CARRO AZ"

// TODO: colocar sizeHint usando tamanho do texto atual?
