#include "widgetcompra.h"
#include "ui_widgetcompra.h"

WidgetCompra::WidgetCompra(QWidget *parent) : Widget(parent), ui(new Ui::WidgetCompra) {
  ui->setupUi(this);

  setConnections();
}

WidgetCompra::~WidgetCompra() { delete ui; }

bool WidgetCompra::updateTables() {
  const QString currentText = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currentText == "Devoluções" and not ui->widgetDevolucao->updateTables()) return false;
  if (currentText == "Resumo" and not ui->widgetResumo->updateTables()) return false;
  if (currentText == "Pendentes" and not ui->widgetPendentes->updateTables()) return false;
  if (currentText == "Gerar Compra" and not ui->widgetGerar->updateTables()) return false;
  if (currentText == "Confirmar Compra" and not ui->widgetConfirmar->updateTables()) return false;
  if (currentText == "Faturamento" and not ui->widgetFaturar->updateTables()) return false;
  if (currentText == "Compras" and not ui->widgetOC->updateTables()) return false;

  return true;
}

void WidgetCompra::on_tabWidget_currentChanged(const int &) { updateTables(); }

void WidgetCompra::setConnections() {
  // REFAC: couldnt I connect directly to Application?

  connect(ui->widgetOC, &WidgetCompraOC::errorSignal, this, &WidgetCompra::errorSignal);
  connect(ui->widgetDevolucao, &WidgetCompraDevolucao::errorSignal, this, &WidgetCompra::errorSignal);
  connect(ui->widgetPendentes, &WidgetCompraPendentes::errorSignal, this, &WidgetCompra::errorSignal);
  connect(ui->widgetGerar, &WidgetCompraGerar::errorSignal, this, &WidgetCompra::errorSignal);
  connect(ui->widgetConfirmar, &WidgetCompraConfirmar::errorSignal, this, &WidgetCompra::errorSignal);
  connect(ui->widgetFaturar, &WidgetCompraFaturar::errorSignal, this, &WidgetCompra::errorSignal);

  connect(ui->widgetOC, &WidgetCompraOC::transactionStarted, this, &WidgetCompra::transactionStarted);
  connect(ui->widgetDevolucao, &WidgetCompraDevolucao::transactionStarted, this, &WidgetCompra::transactionStarted);
  connect(ui->widgetPendentes, &WidgetCompraPendentes::transactionStarted, this, &WidgetCompra::transactionStarted);
  connect(ui->widgetGerar, &WidgetCompraGerar::transactionStarted, this, &WidgetCompra::transactionStarted);
  connect(ui->widgetConfirmar, &WidgetCompraConfirmar::transactionStarted, this, &WidgetCompra::transactionStarted);
  connect(ui->widgetFaturar, &WidgetCompraFaturar::transactionStarted, this, &WidgetCompra::transactionStarted);

  connect(ui->widgetOC, &WidgetCompraOC::transactionEnded, this, &WidgetCompra::transactionEnded);
  connect(ui->widgetDevolucao, &WidgetCompraDevolucao::transactionEnded, this, &WidgetCompra::transactionEnded);
  connect(ui->widgetPendentes, &WidgetCompraPendentes::transactionEnded, this, &WidgetCompra::transactionEnded);
  connect(ui->widgetGerar, &WidgetCompraGerar::transactionEnded, this, &WidgetCompra::transactionEnded);
  connect(ui->widgetConfirmar, &WidgetCompraConfirmar::transactionEnded, this, &WidgetCompra::transactionEnded);
  connect(ui->widgetFaturar, &WidgetCompraFaturar::transactionEnded, this, &WidgetCompra::transactionEnded);

  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &WidgetCompra::on_tabWidget_currentChanged);
}
