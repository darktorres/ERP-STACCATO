#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

#include "doubledelegate.h"
#include "ui_widgetlogisticacaminhao.h"
#include "widgetlogisticacaminhao.h"

WidgetLogisticaCaminhao::WidgetLogisticaCaminhao(QWidget *parent) : Widget(parent), ui(new Ui::WidgetLogisticaCaminhao) {
  ui->setupUi(this);

  connect(ui->table, &TableView::clicked, this, &WidgetLogisticaCaminhao::on_table_clicked);
  connect(ui->table, &TableView::entered, this, &WidgetLogisticaCaminhao::on_table_entered);
}

WidgetLogisticaCaminhao::~WidgetLogisticaCaminhao() { delete ui; }

void WidgetLogisticaCaminhao::setupTables() {
  // REFAC: refactor this to not select in here

  modelCaminhao.setTable("view_caminhao");
  modelCaminhao.setEditStrategy(QSqlTableModel::OnManualSubmit);

  if (not modelCaminhao.select()) { emit errorSignal("Erro lendo tabela caminhâo: " + modelCaminhao.lastError().text()); }

  ui->table->setModel(&modelCaminhao);
  ui->table->hideColumn("idVeiculo");

  modelCarga.setTable("view_caminhao_resumo");
  modelCarga.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelCarga.setHeaderData("data", "Data");

  modelCarga.setFilter("0");

  if (not modelCarga.select()) { return; }

  ui->tableCarga->setModel(&modelCarga);
  ui->tableCarga->hideColumn("idVeiculo");
  ui->tableCarga->setItemDelegateForColumn("Kg", new DoubleDelegate(this));
}

bool WidgetLogisticaCaminhao::updateTables() {
  if (modelCaminhao.tableName().isEmpty()) { setupTables(); }

  }

  ui->table->resizeColumnsToContents();
  if (not modelCaminhao.select()) { return; }

  return true;
}

void WidgetLogisticaCaminhao::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetLogisticaCaminhao::on_table_clicked(const QModelIndex &index) {
  modelCarga.setFilter("idVeiculo = " + modelCaminhao.data(index.row(), "idVeiculo").toString());

  if (not modelCarga.select()) { return; }
}
