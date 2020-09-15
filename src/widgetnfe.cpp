#include "widgetnfe.h"
#include "ui_widgetnfe.h"

WidgetNfe::WidgetNfe(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNfe) {
  ui->setupUi(this);

  connect(ui->tabWidgetNfe, &QTabWidget::currentChanged, this, &WidgetNfe::on_tabWidgetNfe_currentChanged);
}

WidgetNfe::~WidgetNfe() { delete ui; }

void WidgetNfe::updateTables() {
  const QString currentTab = ui->tabWidgetNfe->tabText(ui->tabWidgetNfe->currentIndex());

  if (currentTab == "Entrada") { ui->widgetEntrada->updateTables(); }
  if (currentTab == "Saída") { ui->widgetSaida->updateTables(); }
  if (currentTab == "Distribuição") { ui->widgetDistribuicao->updateTables(); }
}

void WidgetNfe::resetTables() {
  ui->widgetEntrada->resetTables();
  ui->widgetSaida->resetTables();
}

void WidgetNfe::on_tabWidgetNfe_currentChanged(const int) { updateTables(); }
