#include "lineedit.h"

#include <QDebug>

LineEdit::LineEdit(QWidget *parent) : QLineEdit(parent) { connect(this, &QLineEdit::textChanged, this, &LineEdit::resizeToContent); }

void LineEdit::resizeToContent() {
  if (text().isEmpty()) { return setFixedWidth(baseSize().width()); }

  const int fmSize = fontMetrics().boundingRect(text()).width();

  setFixedWidth(qMax(fmSize + 15, baseSize().width()));
}
