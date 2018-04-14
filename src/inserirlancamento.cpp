#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

#include "checkboxdelegate.h"
#include "comboboxdelegate.h"
#include "dateformatdelegate.h"
#include "doubledelegate.h"
#include "inserirlancamento.h"
#include "itembox.h"
#include "itemboxdelegate.h"
#include "lineeditdelegate.h"
#include "reaisdelegate.h"
#include "ui_inserirlancamento.h"

InserirLancamento::InserirLancamento(const Tipo tipo, QWidget *parent) : Dialog(parent), tipo(tipo), ui(new Ui::InserirLancamento) {
  ui->setupUi(this);

  connect(ui->pushButtonCriarLancamento, &QPushButton::clicked, this, &InserirLancamento::on_pushButtonCriarLancamento_clicked);
  connect(ui->pushButtonDuplicarLancamento, &QPushButton::clicked, this, &InserirLancamento::on_pushButtonDuplicarLancamento_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InserirLancamento::on_pushButtonSalvar_clicked);

  setWindowFlags(Qt::Window);

  setupTables();
}

InserirLancamento::~InserirLancamento() { delete ui; }

void InserirLancamento::setupTables() {
  modelContaPagamento.setTable(tipo == Tipo::Pagar ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento");
  modelContaPagamento.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelContaPagamento.setHeaderData("dataEmissao", "Data Emissão");
  modelContaPagamento.setHeaderData("contraParte", "ContraParte");
  modelContaPagamento.setHeaderData("idLoja", "Centro Custo");
  modelContaPagamento.setHeaderData("valor", "R$");
  modelContaPagamento.setHeaderData("tipo", "Tipo");
  modelContaPagamento.setHeaderData("dataPagamento", "Vencimento");
  modelContaPagamento.setHeaderData("observacao", "Obs.");
  modelContaPagamento.setHeaderData("grupo", "Grupo");
  modelContaPagamento.setHeaderData("subGrupo", "SubGrupo");

  modelContaPagamento.setFilter("0");

  if (not modelContaPagamento.select()) { return; }

  ui->table->setModel(&modelContaPagamento);
  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this, 2));
  ui->table->setItemDelegateForColumn("tipo", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagamento, this));
  ui->table->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::StatusReceber, this));
  ui->table->setItemDelegateForColumn("contaDestino", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Conta, this));
  ui->table->setItemDelegateForColumn("representacao", new CheckBoxDelegate(this, true));
  ui->table->setItemDelegateForColumn("idLoja", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, false, this));
  ui->table->setItemDelegateForColumn("grupo", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Grupo, this));
  ui->table->setItemDelegateForColumn("contraParte", new LineEditDelegate(LineEditDelegate::Tipo::ContraPartePagar, this));
  ui->table->setItemDelegateForColumn("dataPagamento", new DateFormatDelegate(this));
  // TODO: 5colocar lineEditDelegate para subgrupo
  ui->table->hideColumn("nfe");
  ui->table->hideColumn("taxa");
  ui->table->hideColumn("parcela");
  ui->table->hideColumn("status");
  ui->table->hideColumn("representacao");
  ui->table->hideColumn("dataRealizado");
  ui->table->hideColumn("valorReal");
  ui->table->hideColumn("tipoReal");
  ui->table->hideColumn("parcelaReal");
  ui->table->hideColumn("contaDestino");
  ui->table->hideColumn("tipoDet");
  ui->table->hideColumn("centroCusto");
  ui->table->hideColumn("comissao");
  ui->table->hideColumn("idPagamento");
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("idVenda");
  ui->table->hideColumn("created");
  ui->table->hideColumn("lastUpdated");
  ui->table->hideColumn("desativado");
}

void InserirLancamento::openPersistentEditor() {
  for (int row = 0; row < modelContaPagamento.rowCount(); ++row) {
    ui->table->openPersistentEditor(row, "idLoja");
    ui->table->openPersistentEditor(row, "tipo");
    ui->table->openPersistentEditor(row, "grupo");
  }
}

void InserirLancamento::on_pushButtonCriarLancamento_clicked() {
  const int newRow = modelContaPagamento.rowCount();
  modelContaPagamento.insertRow(newRow);

  if (not modelContaPagamento.setData(newRow, "status", "PENDENTE")) { return; }
  if (not modelContaPagamento.setData(newRow, "dataEmissao", QDate::currentDate())) { return; }

  openPersistentEditor();
}

void InserirLancamento::on_pushButtonSalvar_clicked() {
  if (not verifyFields()) { return; }

  if (not modelContaPagamento.submitAll()) { return; }

  emit informationSignal("Lançamento salvo com sucesso!");
  close();
}

bool InserirLancamento::verifyFields() {
  for (int row = 0; row < modelContaPagamento.rowCount(); ++row) {
    if (modelContaPagamento.data(row, "dataEmissao").toString().isEmpty()) {
      QMessageBox::critical(this, "Erro!", "Faltou preencher 'Data Emissão' na linha: " + QString::number(row + 1));
      return false;
    }

    if (modelContaPagamento.data(row, "idLoja").toInt() == 0) {
      QMessageBox::critical(this, "Erro!", "Faltou preencher 'Centro Custo' na linha: " + QString::number(row + 1));
      return false;
    }

    if (modelContaPagamento.data(row, "contraParte").toString().isEmpty()) {
      QMessageBox::critical(this, "Erro!", "Faltou preencher 'ContraParte' na linha: " + QString::number(row + 1));
      return false;
    }

    if (modelContaPagamento.data(row, "valor").toString().isEmpty()) {
      QMessageBox::critical(this, "Erro!", "Faltou preencher 'R$' na linha: " + QString::number(row + 1));
      return false;
    }

    if (modelContaPagamento.data(row, "tipo").toString().isEmpty()) {
      QMessageBox::critical(this, "Erro!", "Faltou preencher 'Tipo' na linha: " + QString::number(row + 1));
      return false;
    }

    if (modelContaPagamento.data(row, "dataPagamento").toString().isEmpty()) {
      QMessageBox::critical(this, "Erro!", "Faltou preencher 'Vencimento' na linha: " + QString::number(row + 1));
      return false;
    }

    if (modelContaPagamento.data(row, "grupo").toString().isEmpty()) {
      QMessageBox::critical(this, "Erro!", "Faltou preencher 'Grupo' na linha: " + QString::number(row + 1));
      return false;
    }
  }

  return true;
}

void InserirLancamento::on_pushButtonDuplicarLancamento_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Deve selecionar uma linha primeiro!");
    return;
  }

  const int row = list.first().row();
  const int newRow = modelContaPagamento.rowCount();

  modelContaPagamento.insertRow(newRow);

  for (int col = 0; col < modelContaPagamento.columnCount(); ++col) {
    if (modelContaPagamento.fieldIndex("valor") == col) { continue; }
    if (modelContaPagamento.fieldIndex("desativado") == col) { continue; }
    if (modelContaPagamento.fieldIndex("created") == col) { continue; }
    if (modelContaPagamento.fieldIndex("lastUpdated") == col) { continue; }

    if (not modelContaPagamento.setData(newRow, col, modelContaPagamento.data(row, col))) { return; }
  }

  openPersistentEditor();
}
