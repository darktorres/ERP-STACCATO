#include "combobox.h"

#include "application.h"

ComboBox::ComboBox(QWidget *parent) : QComboBox(parent) {}

QVariant ComboBox::getCurrentValue() const { return currentData(); }

void ComboBox::setCurrentValue(const QVariant &value) {
  for (int index = 0, size = count(); index < size; ++index) {
    if (value == itemData(index)) {
      setCurrentIndex(index);
      return;
    }
  }

  throw RuntimeException("Erro procurando " + value.toString() + "!");
}
