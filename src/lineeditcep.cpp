#include "lineeditcep.h"

LineEditCEP::LineEditCEP(QWidget *parent) : QLineEdit(parent) { setProperty("value", ""); }

bool LineEditCEP::isValid() const { return (text().size() == 9 and text() != "00000-000"); }
