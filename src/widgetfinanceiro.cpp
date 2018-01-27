#include <QDebug>

#include "ui_widgetfinanceiro.h"
#include "widgetfinanceiro.h"

WidgetFinanceiro::WidgetFinanceiro(QWidget *parent) : Widget(parent), ui(new Ui::WidgetFinanceiro) {
  ui->setupUi(this);

  ui->widgetPagar->setTipo(WidgetFinanceiroContas::Tipo::Pagar);
  ui->widgetReceber->setTipo(WidgetFinanceiroContas::Tipo::Receber);
  ui->widgetVenda->setFinanceiro();

  setConnections();
}

WidgetFinanceiro::~WidgetFinanceiro() { delete ui; }

bool WidgetFinanceiro::updateTables() {
  const QString currentText = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currentText == "Fluxo de Caixa") return ui->widgetFluxoCaixa->updateTables();
  if (currentText == "Contas a Pagar") return ui->widgetPagar->updateTables();
  if (currentText == "Contas a Receber") return ui->widgetReceber->updateTables();
  if (currentText == "Vendas") return ui->widgetVenda->updateTables();
  if (currentText == "Compras") return ui->widgetCompra->updateTables();

  return true;
}

void WidgetFinanceiro::setConnections() {
  connect(ui->widgetFluxoCaixa, &WidgetFinanceiroFluxoCaixa::errorSignal, this, &WidgetFinanceiro::errorSignal);
  connect(ui->widgetPagar, &WidgetFinanceiroContas::errorSignal, this, &WidgetFinanceiro::errorSignal);
  connect(ui->widgetReceber, &WidgetFinanceiroContas::errorSignal, this, &WidgetFinanceiro::errorSignal);
  connect(ui->widgetVenda, &WidgetVenda::errorSignal, this, &WidgetFinanceiro::errorSignal);
  connect(ui->widgetCompra, &WidgetFinanceiroCompra::errorSignal, this, &WidgetFinanceiro::errorSignal);

  connect(ui->widgetFluxoCaixa, &WidgetFinanceiroFluxoCaixa::transactionStarted, this, &WidgetFinanceiro::transactionStarted);
  connect(ui->widgetPagar, &WidgetFinanceiroContas::transactionStarted, this, &WidgetFinanceiro::transactionStarted);
  connect(ui->widgetReceber, &WidgetFinanceiroContas::transactionStarted, this, &WidgetFinanceiro::transactionStarted);
  connect(ui->widgetVenda, &WidgetVenda::transactionStarted, this, &WidgetFinanceiro::transactionStarted);
  connect(ui->widgetCompra, &WidgetFinanceiroCompra::transactionStarted, this, &WidgetFinanceiro::transactionStarted);

  connect(ui->widgetFluxoCaixa, &WidgetFinanceiroFluxoCaixa::transactionEnded, this, &WidgetFinanceiro::transactionEnded);
  connect(ui->widgetPagar, &WidgetFinanceiroContas::transactionEnded, this, &WidgetFinanceiro::transactionEnded);
  connect(ui->widgetReceber, &WidgetFinanceiroContas::transactionEnded, this, &WidgetFinanceiro::transactionEnded);
  connect(ui->widgetVenda, &WidgetVenda::transactionEnded, this, &WidgetFinanceiro::transactionEnded);
  connect(ui->widgetCompra, &WidgetFinanceiroCompra::transactionEnded, this, &WidgetFinanceiro::transactionEnded);

  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &WidgetFinanceiro::updateTables);
}

// TODO: 0a cada dia colocar em 'maintenance' um job para enviar o relatorio das financas de 3 dias antes

// select cp.dataEmissao, cp.dataRealizado, cp.valorReal, concat(lhc.banco, ' - ', lhc.agencia, ' - ', lhc.conta) AS
// 'Conta', cp.observacao, cp.contraParte, cp.grupo, cp.subGrupo
// from conta_a_pagar_has_pagamento cp
// left join nfe n on cp.nfe = n.idNFe
// left join loja_has_conta lhc on cp.contaDestino = lhc.idConta
// where cp.dataRealizado is not null and cp.valorReal is not null
// order by cp.dataRealizado;

// select cr.dataEmissao, cr.dataRealizado, cr.valorReal, concat(lhc.banco, ' - ', lhc.agencia, ' - ', lhc.conta) AS
// 'Conta', cr.observacao, cr.contraParte, cr.grupo, cr.subGrupo
// from conta_a_receber_has_pagamento cr
// left join loja_has_conta lhc on cr.contaDestino = lhc.idConta
// where cr.valorReal is not null
// order by cr.dataRealizado;
// TODO: 0poder deixar 'agencia' e 'conta' como nulo nos casos em que nao existem
// TODO: poder refazer o fluxo dos pagamentos parcialmente, refazendo pagamentos individualemente com a condicao do total continuar batendo
// TODO: mostrar na tela de edição de fluxo as notas associadas a compra e poder associar pagamentos com notas
