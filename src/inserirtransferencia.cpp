#include "inserirtransferencia.h"
#include "ui_inserirtransferencia.h"

#include "application.h"

#include <QMessageBox>
#include <QSqlError>

InserirTransferencia::InserirTransferencia(QWidget *parent) : QDialog(parent), ui(new Ui::InserirTransferencia) {
  ui->setupUi(this);

  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &InserirTransferencia::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InserirTransferencia::on_pushButtonSalvar_clicked);

  setupTables();

  ui->itemBoxDe->setSearchDialog(SearchDialog::conta(this));
  ui->itemBoxPara->setSearchDialog(SearchDialog::conta(this));

  ui->dateEdit->setDate(qApp->serverDate());
}

InserirTransferencia::~InserirTransferencia() { delete ui; }

void InserirTransferencia::on_pushButtonSalvar_clicked() {
  verifyFields();

  qApp->startTransaction("InserirTransferencia::on_pushButtonSalvar");

  cadastrar();

  qApp->endTransaction();

  qApp->enqueueInformation("Transferência registrada com sucesso!", this);

  close();
}

void InserirTransferencia::cadastrar() {
  // lancamento 'de'

  const int rowDe = modelDe.insertRowAtEnd();

  modelDe.setData(rowDe, "status", "PAGO");
  modelDe.setData(rowDe, "dataEmissao", ui->dateEdit->date());
  modelDe.setData(rowDe, "idLoja", "1"); // Geral
  modelDe.setData(rowDe, "contraParte", "TRANSFERÊNCIA PARA " + ui->itemBoxPara->text());
  modelDe.setData(rowDe, "valor", ui->doubleSpinBoxValor->value());
  modelDe.setData(rowDe, "tipo", "1. Transf. Banc.");
  modelDe.setData(rowDe, "dataPagamento", ui->dateEdit->date());
  modelDe.setData(rowDe, "dataRealizado", ui->dateEdit->date());
  modelDe.setData(rowDe, "valorReal", ui->doubleSpinBoxValor->value());
  modelDe.setData(rowDe, "tipoReal", "1. Transf. Banc.");
  modelDe.setData(rowDe, "parcelaReal", "1");
  modelDe.setData(rowDe, "idConta", ui->itemBoxDe->getId());
  modelDe.setData(rowDe, "centroCusto", 1); // Geral
  modelDe.setData(rowDe, "grupo", "Transferência");
  modelDe.setData(rowDe, "observacao", ui->lineEditObservacao->text());

  // lancamento 'para'

  const int rowPara = modelPara.insertRowAtEnd();

  modelPara.setData(rowPara, "status", "RECEBIDO");
  modelPara.setData(rowPara, "dataEmissao", ui->dateEdit->date());
  modelPara.setData(rowPara, "idLoja", "1"); // Geral
  modelPara.setData(rowPara, "contraParte", "TRANSFERÊNCIA DE " + ui->itemBoxDe->text());
  modelPara.setData(rowPara, "valor", ui->doubleSpinBoxValor->value());
  modelPara.setData(rowPara, "tipo", "1. Transf. Banc.");
  modelPara.setData(rowPara, "dataPagamento", ui->dateEdit->date());
  modelPara.setData(rowPara, "dataRealizado", ui->dateEdit->date());
  modelPara.setData(rowPara, "valorReal", ui->doubleSpinBoxValor->value());
  modelPara.setData(rowPara, "tipoReal", "1. Transf. Banc.");
  modelPara.setData(rowPara, "parcelaReal", "1");
  modelPara.setData(rowPara, "idConta", ui->itemBoxPara->getId());
  modelPara.setData(rowPara, "centroCusto", 1); // Geral
  modelPara.setData(rowPara, "grupo", "Transferência");
  modelPara.setData(rowPara, "observacao", ui->lineEditObservacao->text());

  modelDe.submitAll();

  modelPara.submitAll();
}

void InserirTransferencia::on_pushButtonCancelar_clicked() { close(); }

void InserirTransferencia::verifyFields() {
  if (ui->itemBoxDe->text().isEmpty()) { throw RuntimeError("Conta 'De' não preenchido!", this); }

  if (ui->itemBoxPara->text().isEmpty()) { throw RuntimeError("Conta 'Para' não preenchido!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxValor->value())) { throw RuntimeError("Valor não preenchido!", this); }

  if (ui->lineEditObservacao->text().isEmpty()) { throw RuntimeError("Preencha a observação!", this); }
}

void InserirTransferencia::setupTables() {
  modelDe.setTable("conta_a_pagar_has_pagamento");

  //-----------------------------------------

  modelPara.setTable("conta_a_receber_has_pagamento");
}

// TODO: 5colocar campo para observacao (na tabela já tem campo observacao, apenas preencher)
// TODO: 5fazer transferencia especial para conta cliente, fazendo a operacao no credito tambem
