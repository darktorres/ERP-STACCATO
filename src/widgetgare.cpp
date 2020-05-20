#include "widgetgare.h"
#include "ui_widgetgare.h"

#include "application.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "xml_viewer.h"

#include <QSqlError>
#include <QSqlQuery>

WidgetGare::WidgetGare(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGare) {
  ui->setupUi(this);

  connect(ui->pushButtonBaixaCNAB, &QPushButton::clicked, this, &WidgetGare::on_pushButtonBaixaCNAB_clicked);
  connect(ui->pushButtonGerarCNAB, &QPushButton::clicked, this, &WidgetGare::on_pushButtonGerarCNAB_clicked);
  connect(ui->table, &TableView::activated, this, &WidgetGare::on_table_activated);
}

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

  model.setFilter("status IN ('PENDENTE GARE', 'LIBERADO GARE')");

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
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));

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

void WidgetGare::on_table_activated(const QModelIndex &index) {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", model.data(index.row(), "idNFe"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando xml da nota: " + query.lastError().text(), this); }

  auto *viewer = new XML_Viewer(query.value("xml").toByteArray(), this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
}
