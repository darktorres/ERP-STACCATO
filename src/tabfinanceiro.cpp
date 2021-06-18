#include "tabfinanceiro.h"
#include "ui_widgetfinanceiro.h"

#include <QDebug>

TabFinanceiro::TabFinanceiro(QWidget *parent) : QWidget(parent), ui(new Ui::TabFinanceiro) {
  ui->setupUi(this);

  ui->widgetPagar->setTipo(WidgetFinanceiroContas::Tipo::Pagar);
  ui->widgetReceber->setTipo(WidgetFinanceiroContas::Tipo::Receber);
  ui->widgetVenda->setFinanceiro();

  setConnections();
}

TabFinanceiro::~TabFinanceiro() { delete ui; }

void TabFinanceiro::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &TabFinanceiro::updateTables, connectionType);
}

void TabFinanceiro::updateTables() {
  const QString currentTab = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currentTab == "Fluxo de Caixa") { ui->widgetFluxoCaixa->updateTables(); }
  if (currentTab == "Contas a Pagar") { ui->widgetPagar->updateTables(); }
  if (currentTab == "Contas a Receber") { ui->widgetReceber->updateTables(); }
  if (currentTab == "GARE") { ui->widgetGare->updateTables(); }
  if (currentTab == "Vendas") { ui->widgetVenda->updateTables(); }
  if (currentTab == "Compras") { ui->widgetCompra->updateTables(); }
}

void TabFinanceiro::resetTables() {
  ui->widgetFluxoCaixa->resetTables();
  ui->widgetPagar->resetTables();
  ui->widgetReceber->resetTables();
  ui->widgetGare->resetTables();
  ui->widgetVenda->resetTables();
  ui->widgetCompra->resetTables();
}

// TODO: 0a cada dia colocar em 'maintenance' um job para enviar o relatorio das financas de 3 dias antes

// select cp.dataEmissao, cp.dataRealizado, cp.valorReal, concat(lhc.banco, ' - ', lhc.agencia, ' - ', lhc.conta) AS
// 'Conta', cp.observacao, cp.contraParte, cp.grupo, cp.subGrupo
// from conta_a_pagar_has_pagamento cp
// left join nfe n on cp.nfe = n.idNFe
// left join loja_has_conta lhc on cp.idConta = lhc.idConta
// where cp.dataRealizado is not null and cp.valorReal is not null
// order by cp.dataRealizado;

// select cr.dataEmissao, cr.dataRealizado, cr.valorReal, concat(lhc.banco, ' - ', lhc.agencia, ' - ', lhc.conta) AS
// 'Conta', cr.observacao, cr.contraParte, cr.grupo, cr.subGrupo
// from conta_a_receber_has_pagamento cr
// left join loja_has_conta lhc on cr.idConta = lhc.idConta
// where cr.valorReal is not null
// order by cr.dataRealizado;
// TODO: 0poder deixar 'agencia' e 'conta' como nulo nos casos em que nao existem
// TODO: poder refazer o fluxo dos pagamentos parcialmente, refazendo pagamentos individualemente com a condicao do total continuar batendo
// TODO: mostrar na tela de edição de fluxo as notas associadas a compra e poder associar pagamentos com notas
