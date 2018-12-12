#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "inserirtransferencia.h"
#include "ui_inserirtransferencia.h"

InserirTransferencia::InserirTransferencia(QWidget *parent) : QDialog(parent), ui(new Ui::InserirTransferencia) {
  ui->setupUi(this);

  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &InserirTransferencia::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InserirTransferencia::on_pushButtonSalvar_clicked);

  setupTables();

  ui->itemBoxDe->setSearchDialog(SearchDialog::conta(this));
  ui->itemBoxPara->setSearchDialog(SearchDialog::conta(this));

  ui->dateEdit->setDate(QDate::currentDate());
}

InserirTransferencia::~InserirTransferencia() { delete ui; }

void InserirTransferencia::on_pushButtonSalvar_clicked() {
  if (not verifyFields()) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not cadastrar()) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation("Transferência registrada com sucesso!", this);
  close();
}

bool InserirTransferencia::cadastrar() {
  // lancamento 'de'

  const int rowDe = modelDe.rowCount();
  modelDe.insertRow(rowDe);

  if (not modelDe.setData(rowDe, "status", "PAGO")) { return false; }
  if (not modelDe.setData(rowDe, "dataEmissao", ui->dateEdit->date())) { return false; }
  if (not modelDe.setData(rowDe, "idLoja", "1")) { return false; } // Geral
  if (not modelDe.setData(rowDe, "contraParte", "TRANSFERÊNCIA PARA " + ui->itemBoxPara->text())) { return false; }
  if (not modelDe.setData(rowDe, "valor", ui->doubleSpinBoxValor->value())) { return false; }
  if (not modelDe.setData(rowDe, "tipo", "1. Transf. Banc.")) { return false; }
  if (not modelDe.setData(rowDe, "dataPagamento", ui->dateEdit->date())) { return false; }
  if (not modelDe.setData(rowDe, "dataRealizado", ui->dateEdit->date())) { return false; }
  if (not modelDe.setData(rowDe, "valorReal", ui->doubleSpinBoxValor->value())) { return false; }
  if (not modelDe.setData(rowDe, "tipoReal", "1. Transf. Banc.")) { return false; }
  if (not modelDe.setData(rowDe, "parcelaReal", "1")) { return false; }
  if (not modelDe.setData(rowDe, "contaDestino", ui->itemBoxDe->getId())) { return false; }
  if (not modelDe.setData(rowDe, "centroCusto", 1)) { return false; } // Geral
  if (not modelDe.setData(rowDe, "grupo", "Transferência")) { return false; }
  if (not modelDe.setData(rowDe, "observacao", ui->lineEditObservacao->text())) { return false; }

  // lancamento 'para'

  const int rowPara = modelPara.rowCount();
  modelPara.insertRow(rowPara);

  if (not modelPara.setData(rowPara, "status", "RECEBIDO")) { return false; }
  if (not modelPara.setData(rowPara, "dataEmissao", ui->dateEdit->date())) { return false; }
  if (not modelPara.setData(rowPara, "idLoja", "1")) { return false; } // Geral
  if (not modelPara.setData(rowPara, "contraParte", "TRANSFERÊNCIA DE " + ui->itemBoxDe->text())) { return false; }
  if (not modelPara.setData(rowPara, "valor", ui->doubleSpinBoxValor->value())) { return false; }
  if (not modelPara.setData(rowPara, "tipo", "1. Transf. Banc.")) { return false; }
  if (not modelPara.setData(rowPara, "dataPagamento", ui->dateEdit->date())) { return false; }
  if (not modelPara.setData(rowPara, "dataRealizado", ui->dateEdit->date())) { return false; }
  if (not modelPara.setData(rowPara, "valorReal", ui->doubleSpinBoxValor->value())) { return false; }
  if (not modelPara.setData(rowPara, "tipoReal", "1. Transf. Banc.")) { return false; }
  if (not modelPara.setData(rowPara, "parcelaReal", "1")) { return false; }
  if (not modelPara.setData(rowPara, "contaDestino", ui->itemBoxPara->getId())) { return false; }
  if (not modelPara.setData(rowPara, "centroCusto", 1)) { return false; } // Geral
  if (not modelPara.setData(rowPara, "grupo", "Transferência")) { return false; }
  if (not modelPara.setData(rowPara, "observacao", ui->lineEditObservacao->text())) { return false; }

  if (not modelDe.submitAll()) { return false; }

  if (not modelPara.submitAll()) { return false; }

  return true;
}

void InserirTransferencia::on_pushButtonCancelar_clicked() { close(); }

bool InserirTransferencia::verifyFields() {
  if (ui->itemBoxDe->text().isEmpty()) { return qApp->enqueueError(false, "Conta 'De' não preenchido!", this); }

  if (ui->itemBoxPara->text().isEmpty()) { return qApp->enqueueError(false, "Conta 'Para' não preenchido!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxValor->value())) { return qApp->enqueueError(false, "Valor não preenchido!", this); }

  if (ui->lineEditObservacao->text().isEmpty()) { return qApp->enqueueError(false, "Preencha a observação!", this); }

  return true;
}

void InserirTransferencia::setupTables() {
  modelDe.setTable("conta_a_pagar_has_pagamento");
  modelDe.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelDe.setFilter("0");

  if (not modelDe.select()) { return; }

  modelPara.setTable("conta_a_receber_has_pagamento");
  modelPara.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelPara.setFilter("0");

  if (not modelPara.select()) { return; }
}

// TODO: 5colocar campo para observacao (na tabela já tem campo observacao, apenas preencher)
// TODO: 5fazer transferencia especial para conta cliente, fazendo a operacao no credito tambem
