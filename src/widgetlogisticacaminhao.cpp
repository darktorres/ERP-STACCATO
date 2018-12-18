#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

#include "doubledelegate.h"
#include "ui_widgetlogisticacaminhao.h"
#include "widgetlogisticacaminhao.h"

WidgetLogisticaCaminhao::WidgetLogisticaCaminhao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaCaminhao) { ui->setupUi(this); }

WidgetLogisticaCaminhao::~WidgetLogisticaCaminhao() { delete ui; }

void WidgetLogisticaCaminhao::setConnections() { connect(ui->table, &TableView::clicked, this, &WidgetLogisticaCaminhao::on_table_clicked); }

void WidgetLogisticaCaminhao::setupTables() {
  modelCaminhao.setTable("view_caminhao");

  modelCaminhao.setFilter("");

  ui->table->setModel(&modelCaminhao);

  ui->table->hideColumn("idVeiculo");

  // -----------------------------------------------------------------

  modelCarga.setTable("view_caminhao_resumo");

  modelCarga.setHeaderData("data", "Data");

  ui->tableCarga->setModel(&modelCarga);

  ui->tableCarga->hideColumn("idVeiculo");

  ui->tableCarga->setItemDelegateForColumn("Kg", new DoubleDelegate(this));
}

void WidgetLogisticaCaminhao::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  if (not modelCaminhao.select()) { return; }

  ui->table->resizeColumnsToContents();

  if (not modelCarga.select()) { return; }

  ui->tableCarga->resizeColumnsToContents();
}

void WidgetLogisticaCaminhao::resetTables() { modelIsSet = false; }

void WidgetLogisticaCaminhao::on_table_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  modelCarga.setFilter("idVeiculo = " + modelCaminhao.data(index.row(), "idVeiculo").toString() + " ORDER BY data DESC");

  ui->tableCarga->resizeColumnsToContents();
}
