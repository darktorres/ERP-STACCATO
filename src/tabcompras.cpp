#include "tabcompras.h"
#include "ui_tabcompras.h"

#include "permissao.h"

TabCompras::TabCompras(QWidget *parent) : QWidget(parent), ui(new Ui::TabCompras) {
  ui->setupUi(this);
  setConnections();

  // Marca as telas-filhas antes de aplicar: a poda de aplicarTela para nessas subarvores
  // (cada uma tem prefixo proprio), entao aqui so as paginas do QTabWidget sao gateadas.
  ui->widgetDevolucao->setProperty("prefixoPermissao", "compras.devolucoes");
  ui->widgetResumo->setProperty("prefixoPermissao", "compras.resumo");
  ui->widgetPendentes->setProperty("prefixoPermissao", "compras.pendentes");
  ui->widgetGerar->setProperty("prefixoPermissao", "compras.comprar");
  ui->widgetConfirmar->setProperty("prefixoPermissao", "compras.confirmar");
  ui->widgetFaturar->setProperty("prefixoPermissao", "compras.faturamento");
  ui->widgetOC->setProperty("prefixoPermissao", "compras.consumos");
  ui->widgetHistorico->setProperty("prefixoPermissao", "compras.historico");
  ui->widgetFinanceiro->setProperty("prefixoPermissao", "compras.financeiro");
  ui->widgetCompraAvulsa->setProperty("prefixoPermissao", "compras.avulso");

  Permissao::aplicarTela(this, "compras");

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
  ui->widgetFinanceiro->resetTables();
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
  if (currentTab == "Financeiro") { ui->widgetFinanceiro->updateTables(); }
  if (currentTab == "Avulso") { ui->widgetCompraAvulsa->updateTables(); }
}

void TabCompras::on_tabWidget_currentChanged() { updateTables(); }

void TabCompras::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->widgetGerar, &WidgetCompraGerar::finished, this, [&] { ui->tabWidget->setCurrentWidget(ui->tabPendentes); }, connectionType);
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &TabCompras::on_tabWidget_currentChanged, connectionType);
}
