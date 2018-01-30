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

void WidgetNfe::setConnections() {
  connect(ui->widgetEntrada, &WidgetNfeEntrada::errorSignal, this, &WidgetNfe::errorSignal);
  connect(ui->widgetSaida, &WidgetNfeSaida::errorSignal, this, &WidgetNfe::errorSignal);

  connect(ui->widgetEntrada, &WidgetNfeEntrada::transactionStarted, this, &WidgetNfe::transactionStarted);
  connect(ui->widgetSaida, &WidgetNfeSaida::transactionStarted, this, &WidgetNfe::transactionStarted);

  connect(ui->widgetEntrada, &WidgetNfeEntrada::transactionEnded, this, &WidgetNfe::transactionEnded);
  connect(ui->widgetSaida, &WidgetNfeSaida::transactionEnded, this, &WidgetNfe::transactionEnded);

  connect(ui->tabWidgetNfe, &QTabWidget::currentChanged, this, &WidgetNfe::on_tabWidgetNfe_currentChanged);
}

bool WidgetNfe::updateTables() {
  const QString currentText = ui->tabWidgetNfe->tabText(ui->tabWidgetNfe->currentIndex());

  if (currentText == "Entrada" and not ui->widgetEntrada->updateTables()) return false;
  if (currentText == "Saída" and not ui->widgetSaida->updateTables()) return false;

  return true;
}

void WidgetNfe::on_tabWidgetNfe_currentChanged(const int) { updateTables(); }
