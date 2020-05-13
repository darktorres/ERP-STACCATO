#include "widgetgare.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "ui_widgetgare.h"

WidgetGare::WidgetGare(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGare) { ui->setupUi(this); }

WidgetGare::~WidgetGare() { delete ui; }

void WidgetGare::resetTables() { modelIsSet = false; }

void WidgetGare::updateTables() {
  if (not isSet) {
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  if (not model.select()) { return; }
}

void WidgetGare::setupTables() {
  model.setTable("conta_a_pagar_has_pagamento");

  model.setFilter("status = 'PENDENTE GARE'");

  model.setHeaderData("dataEmissao", "Data Emissão");
  model.setHeaderData("nfe", "NFe");
  model.setHeaderData("valor", "R$");
  model.setHeaderData("tipo", "Tipo");
  model.setHeaderData("parcela", "Parcela");
  model.setHeaderData("dataPagamento", "Data Pgt.");
  model.setHeaderData("observacao", "Obs.");
  model.setHeaderData("status", "Status");
  model.setHeaderData("dataRealizado", "Data Realizado");
  model.setHeaderData("valorReal", "R$ Realizado");

  ui->table->setModel(&model);

  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this, 2));

  ui->table->hideColumn("idPagamento");
  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("contraParte");
  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("tipoReal");
  ui->table->hideColumn("parcelaReal");
  ui->table->hideColumn("contaDestino");
  ui->table->hideColumn("tipoDet");
  ui->table->hideColumn("centroCusto");
  ui->table->hideColumn("grupo");
  ui->table->hideColumn("subGrupo");
  ui->table->hideColumn("desativado");
}

void WidgetGare::on_pushButtonGerarCNAB_clicked() {
  //
}

void WidgetGare::on_pushButtonBaixaCNAB_clicked() {
  //
}
