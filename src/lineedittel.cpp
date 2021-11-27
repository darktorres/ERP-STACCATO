#include "lineedittel.h"

LineEditTel::LineEditTel(QWidget *parent) : QLineEdit(parent) {
  setPlaceholderText("(99)99999-9999");
  setConnections();
}

void LineEditTel::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(this, &QLineEdit::textEdited, this, &LineEditTel::processTel, connectionType);
}

void LineEditTel::processTel(const QString &value) {
  if (value.isEmpty()) { return; }

  QString temp;

  for (const auto &c : value) {
    if (c.isNumber()) { temp += c; }
  }

  temp = temp.left(11);

  const int size = temp.size();

  if (size > 0) { temp.insert(0, '('); }
  if (size > 2) { temp.insert(3, ')'); }
  if (size > 6) { temp.insert(8, '-'); }
  if (size > 10) { qSwap(temp.data()[8], temp.data()[9]); }

  setText(temp);
}
