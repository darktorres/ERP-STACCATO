#include "inserirtransferencia.h"
#include "ui_inserirtransferencia.h"

#include "application.h"
#include "sqlquery.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

InserirTransferencia::InserirTransferencia(QWidget *parent) : QDialog(parent), ui(new Ui::InserirTransferencia) {
  ui->setupUi(this);

  setupTables();

  ui->itemBoxDe->setSearchDialog(SearchDialog::conta(this));
  ui->itemBoxPara->setSearchDialog(SearchDialog::conta(this));
  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));

  ui->frameCliente->hide();

  ui->dateEdit->setDate(qApp->serverDate());

  setConnections();
}

InserirTransferencia::~InserirTransferencia() { delete ui; }

void InserirTransferencia::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->itemBoxCliente, &ItemBox::textChanged, this, &InserirTransferencia::on_itemBoxCliente_textChanged, connectionType);
  connect(ui->itemBoxDe, &ItemBox::textChanged, this, &InserirTransferencia::on_itemBoxPara_textChanged, connectionType);
  connect(ui->itemBoxPara, &ItemBox::textChanged, this, &InserirTransferencia::on_itemBoxPara_textChanged, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &InserirTransferencia::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InserirTransferencia::on_pushButtonSalvar_clicked, connectionType);
}

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

  if (ui->frameCliente->isVisible()) { modelDe.setData(rowDe, "observacao", ui->itemBoxCliente->text() + " - " + ui->lineEditObservacao->text()); }

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

  if (ui->frameCliente->isVisible()) { modelPara.setData(rowDe, "observacao", ui->itemBoxCliente->text() + " - " + ui->lineEditObservacao->text()); }

  modelDe.submitAll();

  modelPara.submitAll();

  if (ui->itemBoxDe->text() == "CRÉDITO DE CLIENTES") {
    SqlQuery query;

    if (not query.exec("UPDATE cliente SET credito = credito - " + QString::number(ui->doubleSpinBoxValor->value()) + " WHERE idCliente = " + ui->itemBoxCliente->getId().toString())) {
      throw RuntimeException("Erro atualizando crédito do cliente: " + query.lastError().text());
    }
  }

  if (ui->itemBoxPara->text() == "CRÉDITO DE CLIENTES") {
    SqlQuery query;

    if (not query.exec("UPDATE cliente SET credito = credito + " + QString::number(ui->doubleSpinBoxValor->value()) + " WHERE idCliente = " + ui->itemBoxCliente->getId().toString())) {
      throw RuntimeException("Erro atualizando crédito do cliente: " + query.lastError().text());
    }
  }
}

void InserirTransferencia::on_pushButtonCancelar_clicked() { close(); }

void InserirTransferencia::verifyFields() {
  if (ui->itemBoxDe->text() == ui->itemBoxPara->text()) { throw RuntimeError("Transferência para a mesma conta!", this); }

  if (ui->itemBoxDe->text().isEmpty()) { throw RuntimeError("Conta 'De' não preenchido!", this); }

  if (ui->itemBoxPara->text().isEmpty()) { throw RuntimeError("Conta 'Para' não preenchido!", this); }

  if (ui->frameCliente->isVisible() and ui->itemBoxCliente->text().isEmpty()) { throw RuntimeError("Não selecionou cliente!"); }

  if (qFuzzyIsNull(ui->doubleSpinBoxValor->value())) { throw RuntimeError("Valor não preenchido!", this); }

  if (ui->lineEditObservacao->text().isEmpty()) { throw RuntimeError("Preencha a observação!", this); }
}

void InserirTransferencia::setupTables() {
  modelDe.setTable("conta_a_pagar_has_pagamento");

  //-----------------------------------------

  modelPara.setTable("conta_a_receber_has_pagamento");
}

void InserirTransferencia::on_itemBoxDe_textChanged(const QString &) {
  ui->itemBoxCliente->clear();

  const bool mostrarCliente = (ui->itemBoxDe->text() == "CRÉDITO DE CLIENTES" or ui->itemBoxPara->text() == "CRÉDITO DE CLIENTES");

  ui->frameCliente->setVisible(mostrarCliente);

  ui->doubleSpinBoxValor->setMaximum(999999.990000);
}

void InserirTransferencia::on_itemBoxPara_textChanged(const QString &) {
  ui->itemBoxCliente->clear();

  const bool mostrarCliente = (ui->itemBoxDe->text() == "CRÉDITO DE CLIENTES" or ui->itemBoxPara->text() == "CRÉDITO DE CLIENTES");

  ui->frameCliente->setVisible(mostrarCliente);

  ui->doubleSpinBoxValor->setMaximum(999999.990000);
}

void InserirTransferencia::on_itemBoxCliente_textChanged(const QString &text) {
  if (text.isEmpty()) {
    ui->doubleSpinBoxValor->setMaximum(0);
    ui->doubleSpinBoxValor->setValue(0);
    return;
  }

  buscarCreditoCliente();
}

void InserirTransferencia::buscarCreditoCliente() {
  if (ui->itemBoxCliente->text().isEmpty()) { return; }

  SqlQuery query;

  if (not query.exec("SELECT credito FROM cliente WHERE idCliente = " + ui->itemBoxCliente->getId().toString())) {
    throw RuntimeException("Erro buscando crédito do cliente: " + query.lastError().text());
  }

  if (not query.first()) { throw RuntimeException("Dados do cliente não encontrados!"); }

  const bool deCliente = (ui->itemBoxDe->text() == "CRÉDITO DE CLIENTES");
  const double credito = query.value("credito").toDouble();

  ui->doubleSpinBoxValor->setMaximum(deCliente ? credito : 999999.990000);
  ui->doubleSpinBoxValor->setValue(query.value("credito").toDouble());

  if (deCliente and qFuzzyIsNull(credito)) { throw RuntimeError("Cliente selecionado não possui crédito!"); }
}
