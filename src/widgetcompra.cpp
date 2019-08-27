#include "widgetcompra.h"
#include "ui_widgetcompra.h"

WidgetCompra::WidgetCompra(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompra) {
  ui->setupUi(this);

  setConnections();
}

WidgetCompra::~WidgetCompra() { delete ui; }

void WidgetCompra::resetTables() {
  ui->widgetDevolucao->resetTables();
  ui->widgetResumo->resetTables();
  ui->widgetPendentes->resetTables();
  ui->widgetGerar->resetTables();
  ui->widgetConfirmar->resetTables();
  ui->widgetFaturar->resetTables();
  ui->widgetOC->resetTables();
}

void WidgetCompra::updateTables() {
  const QString currentText = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currentText == "Devoluções") { ui->widgetDevolucao->updateTables(); }
  if (currentText == "Resumo") { ui->widgetResumo->updateTables(); }
  if (currentText == "Pendentes") { ui->widgetPendentes->updateTables(); }
  if (currentText == "Gerar Compra") { ui->widgetGerar->updateTables(); }
  if (currentText == "Confirmar Compra") { ui->widgetConfirmar->updateTables(); }
  if (currentText == "Faturamento") { ui->widgetFaturar->updateTables(); }
  if (currentText == "Compras") { ui->widgetOC->updateTables(); }
}

void WidgetCompra::on_tabWidget_currentChanged(const int &) { updateTables(); }

void WidgetCompra::setConnections() {
  connect(ui->widgetConfirmar, &WidgetCompraConfirmar::finished, this, [&] { ui->tabWidget->setCurrentWidget(ui->tabPendentes); });
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &WidgetCompra::on_tabWidget_currentChanged);
}
