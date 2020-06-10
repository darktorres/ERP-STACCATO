#include "inserirlancamento.h"
#include "ui_inserirlancamento.h"

#include "application.h"
#include "checkboxdelegate.h"
#include "comboboxdelegate.h"
#include "dateformatdelegate.h"
#include "doubledelegate.h"
#include "itembox.h"
#include "itemboxdelegate.h"
#include "lineeditdelegate.h"
#include "reaisdelegate.h"

#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QtMath>

InserirLancamento::InserirLancamento(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InserirLancamento) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  setConnections();
}

InserirLancamento::~InserirLancamento() { delete ui; }

void InserirLancamento::setupTables() {
  if (tipo == Tipo::Pagar) { modelContaPagamento.setTable("conta_a_pagar_has_pagamento"); }
  if (tipo == Tipo::Receber) { modelContaPagamento.setTable("conta_a_receber_has_pagamento"); }

  modelContaPagamento.setHeaderData("dataEmissao", "Data Emissão");
  modelContaPagamento.setHeaderData("idLoja", "Centro Custo");
  modelContaPagamento.setHeaderData("contraParte", "ContraParte");
  modelContaPagamento.setHeaderData("nfe", "NFe");
  modelContaPagamento.setHeaderData("valor", "R$");
  modelContaPagamento.setHeaderData("tipo", "Tipo");
  modelContaPagamento.setHeaderData("parcela", "Parcela");
  modelContaPagamento.setHeaderData("dataPagamento", "Vencimento");
  modelContaPagamento.setHeaderData("observacao", "Obs.");
  modelContaPagamento.setHeaderData("status", "Status");
  modelContaPagamento.setHeaderData("dataRealizado", "Data Realizado");
  modelContaPagamento.setHeaderData("valorReal", "R$ Real");
  modelContaPagamento.setHeaderData("tipoReal", "Tipo Real");
  modelContaPagamento.setHeaderData("parcelaReal", "Parcela Real");
  modelContaPagamento.setHeaderData("idConta", "Conta");
  modelContaPagamento.setHeaderData("tipoDet", "Tipo Det");
  modelContaPagamento.setHeaderData("grupo", "Grupo");
  modelContaPagamento.setHeaderData("subGrupo", "SubGrupo");

  ui->table->setModel(&modelContaPagamento);

  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("tipo", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagamento, this));

  if (tipo == Tipo::Pagar) { ui->table->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagar, this)); }
  if (tipo == Tipo::Receber) { ui->table->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Receber, this)); }

  ui->table->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, false, this));
  ui->table->setItemDelegateForColumn("idLoja", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, false, this));
  ui->table->setItemDelegateForColumn("grupo", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Grupo, this));
  ui->table->setItemDelegateForColumn("contraParte", new LineEditDelegate(LineEditDelegate::Tipo::ContraPartePagar, this));
  ui->table->setItemDelegateForColumn("dataPagamento", new DateFormatDelegate(this));
  ui->table->setItemDelegateForColumn("dataRealizado", new DateFormatDelegate(this));
  // TODO: 5colocar lineEditDelegate para subgrupo

  ui->table->setPersistentColumns({"idLoja", "tipo", "grupo", "status", "idConta"});

  ui->table->hideColumn("centroCusto");
  ui->table->hideColumn("idPagamento");

  if (tipo == Tipo::Pagar) {
    ui->table->hideColumn("idCompra");
    ui->table->hideColumn("idCnab");
    ui->table->hideColumn("idNFe");
  }

  if (tipo == Tipo::Receber) {
    ui->table->hideColumn("taxa");
    ui->table->hideColumn("comissao");
    ui->table->hideColumn("representacao");
  }

  ui->table->hideColumn("idVenda");
  ui->table->hideColumn("desativado");
}

void InserirLancamento::on_pushButtonCriarLancamento_clicked() {
  unsetConnections();

  [&] {
    const int newRow = modelContaPagamento.insertRowAtEnd();

    if (not modelContaPagamento.setData(newRow, "status", "PENDENTE")) { return; }
    if (not modelContaPagamento.setData(newRow, "dataEmissao", qApp->serverDate())) { return; }
  }();

  setConnections();
}

void InserirLancamento::on_pushButtonSalvar_clicked() {
  if (not verifyFields()) { return; }

  if (not modelContaPagamento.submitAll()) { return; }

  qApp->enqueueInformation("Lançamento salvo com sucesso!", this);
  close();
}

bool InserirLancamento::verifyFields() {
  for (int row = 0; row < modelContaPagamento.rowCount(); ++row) {
    if (modelContaPagamento.data(row, "dataEmissao").toDate().isNull()) { return qApp->enqueueError(false, "Faltou preencher 'Data Emissão' na linha: " + QString::number(row + 1), this); }

    if (modelContaPagamento.data(row, "idLoja").toUInt() == 0) { return qApp->enqueueError(false, "Faltou preencher 'Centro Custo' na linha: " + QString::number(row + 1), this); }

    if (modelContaPagamento.data(row, "contraParte").toString().isEmpty()) { return qApp->enqueueError(false, "Faltou preencher 'ContraParte' na linha: " + QString::number(row + 1), this); }

    if (qFuzzyIsNull(modelContaPagamento.data(row, "valor").toDouble())) { return qApp->enqueueError(false, "Faltou preencher 'R$' na linha: " + QString::number(row + 1), this); }

    if (modelContaPagamento.data(row, "tipo").toString().isEmpty()) { return qApp->enqueueError(false, "Faltou preencher 'Tipo' na linha: " + QString::number(row + 1), this); }

    if (modelContaPagamento.data(row, "dataPagamento").toDate().isNull()) { return qApp->enqueueError(false, "Faltou preencher 'Vencimento' na linha: " + QString::number(row + 1), this); }

    if (modelContaPagamento.data(row, "grupo").toString().isEmpty()) { return qApp->enqueueError(false, "Faltou preencher 'Grupo' na linha: " + QString::number(row + 1), this); }
  }

  return true;
}

void InserirLancamento::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonCriarLancamento, &QPushButton::clicked, this, &InserirLancamento::on_pushButtonCriarLancamento_clicked, connectionType);
  connect(ui->pushButtonDuplicarLancamento, &QPushButton::clicked, this, &InserirLancamento::on_pushButtonDuplicarLancamento_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InserirLancamento::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &InserirLancamento::preencher, connectionType);
}

void InserirLancamento::unsetConnections() {
  disconnect(ui->pushButtonCriarLancamento, &QPushButton::clicked, this, &InserirLancamento::on_pushButtonCriarLancamento_clicked);
  disconnect(ui->pushButtonDuplicarLancamento, &QPushButton::clicked, this, &InserirLancamento::on_pushButtonDuplicarLancamento_clicked);
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InserirLancamento::on_pushButtonSalvar_clicked);
  disconnect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &InserirLancamento::preencher);
}

void InserirLancamento::on_pushButtonDuplicarLancamento_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Deve selecionar uma linha primeiro!", this); }

  const int row = list.first().row();
  const int newRow = modelContaPagamento.insertRowAtEnd();

  for (int col = 0; col < modelContaPagamento.columnCount(); ++col) {
    if (modelContaPagamento.fieldIndex("nfe") == col) { continue; }
    if (modelContaPagamento.fieldIndex("valor") == col) { continue; }
    if (modelContaPagamento.fieldIndex("desativado") == col) { continue; }
    if (modelContaPagamento.fieldIndex("created") == col) { continue; }
    if (modelContaPagamento.fieldIndex("lastUpdated") == col) { continue; }

    const QVariant value = modelContaPagamento.data(row, col);

    if (not modelContaPagamento.setData(newRow, col, value)) { return; }
  }
}

void InserirLancamento::preencher(const QModelIndex &index) {
  unsetConnections();

  [&] {
    const int row = index.row();

    if (index.column() == ui->table->columnIndex("dataRealizado")) {
      const int contaSantander = 3;
      const int contaItau = 33;

      const int idConta = (tipo == Tipo::Receber and modelContaPagamento.data(row, "tipo").toString().contains("BOLETO")) ? contaItau : contaSantander;

      if (modelContaPagamento.data(row, "idConta").toInt() == 0) {
        if (not modelContaPagamento.setData(row, "idConta", idConta)) { return; }
      }

      if (not modelContaPagamento.setData(row, "status", (tipo == Tipo::Receber) ? "RECEBIDO" : "PAGO")) { return; }
      if (not modelContaPagamento.setData(row, "valorReal", modelContaPagamento.data(row, "valor"))) { return; }
      if (not modelContaPagamento.setData(row, "tipoReal", modelContaPagamento.data(row, "tipo"))) { return; }
      if (not modelContaPagamento.setData(row, "parcelaReal", modelContaPagamento.data(row, "parcela"))) { return; }
      if (not modelContaPagamento.setData(row, "centroCusto", modelContaPagamento.data(row, "idLoja"))) { return; }
    }
  }();

  setConnections();
}
