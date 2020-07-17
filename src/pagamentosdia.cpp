#include "pagamentosdia.h"
#include "ui_pagamentosdia.h"

#include "reaisdelegate.h"

#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

PagamentosDia::PagamentosDia(QWidget *parent) : QDialog(parent), ui(new Ui::PagamentosDia) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();
}

PagamentosDia::~PagamentosDia() { delete ui; }

void PagamentosDia::setupTables() {
  modelViewFluxoCaixa.setTable("view_fluxo_caixa_realizado");

  modelViewFluxoCaixa.setHeaderData("contaDestino", "Conta Destino");

  ui->tableView->setModel(&modelViewFluxoCaixa);

  // TODO: 5usar outra coluna no lugar de idCompra?
  // TODO: 5renomear/esconder colunas de data
  // TODO: 5colocar delegates

  ui->tableView->hideColumn("idConta");
  ui->tableView->hideColumn("dataRealizado");

  ui->tableView->setItemDelegateForColumn("R$", new ReaisDelegate(this));
}

bool PagamentosDia::setFilter(const QDate &date, const QString &idConta) {
  const QString filtroConta = idConta.isEmpty() ? "" : "AND idConta = " + idConta;

  modelViewFluxoCaixa.setFilter("`dataRealizado` = '" + date.toString("yyyy-MM-dd") + "' AND status IN ('PAGO', 'PAGO GARE', 'RECEBIDO') " + filtroConta);

  if (not modelViewFluxoCaixa.select()) { return false; }

  setWindowTitle(date.toString("dd/MM/yyyy"));

  return true;
}
