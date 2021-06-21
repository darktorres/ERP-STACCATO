#include "tabcompras.h"
#include "ui_tabcompras.h"

TabCompras::TabCompras(QWidget *parent) : QWidget(parent), ui(new Ui::TabCompras) {
  ui->setupUi(this);
  setConnections();
}

TabCompras::~TabCompras() { delete ui; }

void TabCompras::resetTables() {
  ui->widgetDevolucao->resetTables();
  ui->widgetResumo->resetTables();
  ui->widgetPendentes->resetTables();
  ui->widgetGerar->resetTables();
  ui->widgetConfirmar->resetTables();
  ui->widgetFaturar->resetTables();
  ui->widgetOC->resetTables();
  ui->widgetHistorico->resetTables();
}

void TabCompras::updateTables() {
  const QString currentTab = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currentTab == "Devoluções") { ui->widgetDevolucao->updateTables(); }
  if (currentTab == "Resumo") { ui->widgetResumo->updateTables(); }
  if (currentTab == "Pendentes") { ui->widgetPendentes->updateTables(); }
  if (currentTab == "Gerar Compra") { ui->widgetGerar->updateTables(); }
  if (currentTab == "Confirmar Compra") { ui->widgetConfirmar->updateTables(); }
  if (currentTab == "Faturamento") { ui->widgetFaturar->updateTables(); }
  if (currentTab == "Consumos") { ui->widgetOC->updateTables(); }
  if (currentTab == "Histórico") { ui->widgetHistorico->updateTables(); }
}

void TabCompras::on_tabWidget_currentChanged(const int &) { updateTables(); }

void TabCompras::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(
      ui->widgetGerar, &WidgetCompraGerar::finished, this, [&] { ui->tabWidget->setCurrentWidget(ui->tabPendentes); }, connectionType);
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &TabCompras::on_tabWidget_currentChanged, connectionType);
}
