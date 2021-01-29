#include "widgetlogisticacaminhao.h"
#include "ui_widgetlogisticacaminhao.h"

#include "doubledelegate.h"

#include <QDebug>
#include <QSqlError>

WidgetLogisticaCaminhao::WidgetLogisticaCaminhao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaCaminhao) { ui->setupUi(this); }

WidgetLogisticaCaminhao::~WidgetLogisticaCaminhao() { delete ui; }

void WidgetLogisticaCaminhao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxDesativados, &QCheckBox::toggled, this, &WidgetLogisticaCaminhao::on_checkBoxDesativados_toggled, connectionType);
  connect(ui->table, &TableView::clicked, this, &WidgetLogisticaCaminhao::on_table_clicked, connectionType);
}

void WidgetLogisticaCaminhao::setupTables() {
  modelCaminhao.setTable("view_caminhao");

  modelCaminhao.setFilter("desativado = FALSE");

  ui->table->setModel(&modelCaminhao);

  ui->table->hideColumn("idVeiculo");
  ui->table->hideColumn("desativado");

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

  modelCaminhao.select();

  modelCarga.select();
}

void WidgetLogisticaCaminhao::resetTables() { modelIsSet = false; }

void WidgetLogisticaCaminhao::on_table_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  modelCarga.setFilter("idVeiculo = " + modelCaminhao.data(index.row(), "idVeiculo").toString());
}

void WidgetLogisticaCaminhao::on_checkBoxDesativados_toggled(const bool checked) { modelCaminhao.setFilter(checked ? "" : "desativado = FALSE"); }
