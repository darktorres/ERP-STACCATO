#include "pagamentosdia.h"
#include "ui_pagamentosdia.h"

#include "application.h"
#include "reaisdelegate.h"

#include <QDate>
#include <QDebug>
#include <QSqlError>

PagamentosDia::PagamentosDia(QWidget *parent) : QDialog(parent), ui(new Ui::PagamentosDia) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  connect(ui->tableView, &TableView::doubleClicked, this, &PagamentosDia::on_tableView_doubleClicked);
}

PagamentosDia::~PagamentosDia() { delete ui; }

void PagamentosDia::setupTables() {
  modelFluxoCaixa.setTable("view_fluxo_caixa_realizado");

  modelFluxoCaixa.setHeaderData("contaDestino", "Conta Destino");

  ui->tableView->setModel(&modelFluxoCaixa);

  ui->tableView->hideColumn("idConta");
  ui->tableView->hideColumn("dataRealizado");

  ui->tableView->setItemDelegateForColumn("R$", new ReaisDelegate(this));
}

void PagamentosDia::setFilter(const QDate date, const QString &idConta) {
  const QString filtroConta = idConta.isEmpty() ? "" : "AND idConta = " + idConta;

  modelFluxoCaixa.setFilter("`dataRealizado` = '" + date.toString("yyyy-MM-dd") + "' AND status IN ('PAGO', 'PAGO GARE', 'RECEBIDO') " + filtroConta);

  modelFluxoCaixa.select();

  setWindowTitle(date.toString("dd/MM/yyyy"));
}

void PagamentosDia::on_tableView_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelFluxoCaixa.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Compra/Venda") { return qApp->abrirVenda(modelFluxoCaixa.data(index.row(), "Compra/Venda")); }
}
