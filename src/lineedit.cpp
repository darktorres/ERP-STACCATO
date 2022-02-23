#include "lineedit.h"

#include <QDebug>

using namespace std::chrono_literals;

LineEdit::LineEdit(QWidget *parent) : QLineEdit(parent) { setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed); }

void LineEdit::resizeToContent() {
  const int fmSize = fontMetrics().boundingRect(text()).width();

  setMinimumWidth(fmSize + 15);
}

void LineEdit::setDelayed() {
  timer.setSingleShot(true);

  connect(&timer, &QTimer::timeout, this, [&] { emit delayedTextChanged(text()); });
  connect(this, &QLineEdit::textChanged, this, [&] { timer.start(500ms); });
}

void LineEdit::setResizeToText() { connect(this, &QLineEdit::textChanged, this, &LineEdit::resizeToContent); }

// TODO: alterar para que texto grande seja sempre cortado pela direita e não pela esquerda, ex:
// errado: "RRO AZUL"
// certo:  "CARRO AZ"
// para isso usar QLineEdit::home(false)

// TODO: colocar sizeHint usando tamanho do texto atual?
