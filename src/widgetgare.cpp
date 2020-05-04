#include "widgetgare.h"
#include "ui_widgetgare.h"

WidgetGare::WidgetGare(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGare) { ui->setupUi(this); }

WidgetGare::~WidgetGare() { delete ui; }

void WidgetGare::resetTables() { modelIsSet = false; }

void WidgetGare::updateTables() {
  if (not isSet) {
    //
    isSet = true;
  }

  if (not modelIsSet) {
    //
    modelIsSet = true;
  }
}

void WidgetGare::setupTables() {
  //
  //
}
