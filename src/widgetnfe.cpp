#include <QFileDialog>
#include <QMessageBox>

#include "ui_widgetnfe.h"
#include "widgetnfe.h"
#include "xml_viewer.h"

WidgetNfe::WidgetNfe(QWidget *parent) : Widget(parent), ui(new Ui::WidgetNfe) {
  ui->setupUi(this);

  setConnections();
}

WidgetNfe::~WidgetNfe() { delete ui; }

void WidgetNfe::setConnections() { connect(ui->tabWidgetNfe, &QTabWidget::currentChanged, this, &WidgetNfe::on_tabWidgetNfe_currentChanged); }

void WidgetNfe::updateTables() {
  const QString currentText = ui->tabWidgetNfe->tabText(ui->tabWidgetNfe->currentIndex());

  if (currentText == "Entrada") { ui->widgetEntrada->updateTables(); }
  if (currentText == "Saída") { ui->widgetSaida->updateTables(); }
}

  return true;
}

void WidgetNfe::on_tabWidgetNfe_currentChanged(const int) { updateTables(); }
