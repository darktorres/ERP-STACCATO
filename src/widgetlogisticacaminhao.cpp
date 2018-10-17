#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

#include "doubledelegate.h"
#include "ui_widgetlogisticacaminhao.h"
#include "widgetlogisticacaminhao.h"

WidgetLogisticaCaminhao::WidgetLogisticaCaminhao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaCaminhao) { ui->setupUi(this); }

WidgetLogisticaCaminhao::~WidgetLogisticaCaminhao() { delete ui; }

void WidgetLogisticaCaminhao::setConnections() {
  connect(ui->table, &TableView::clicked, this, &WidgetLogisticaCaminhao::on_table_clicked);
  connect(ui->table, &TableView::entered, this, &WidgetLogisticaCaminhao::on_table_entered);
}

void WidgetLogisticaCaminhao::setupTables() {
  modelCaminhao.setTable("view_caminhao");
  modelCaminhao.setEditStrategy(QSqlTableModel::OnManualSubmit);

  ui->table->setModel(&modelCaminhao);
  ui->table->hideColumn("idVeiculo");

  // -----------------------------------------------------------------

  modelCarga.setTable("view_caminhao_resumo");
  modelCarga.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelCarga.setHeaderData("data", "Data");

  modelCarga.setFilter("0");

  if (not modelCarga.select()) { return; }

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
}

void WidgetLogisticaCaminhao::resetTables() { modelIsSet = false; }

void WidgetLogisticaCaminhao::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetLogisticaCaminhao::on_table_clicked(const QModelIndex &index) {
  modelCarga.setFilter("idVeiculo = " + modelCaminhao.data(index.row(), "idVeiculo").toString() + " ORDER BY data DESC");

  if (not modelCarga.select()) { return; }
}
