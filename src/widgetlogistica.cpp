#include "widgetlogistica.h"
#include "ui_widgetlogistica.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

WidgetLogistica::WidgetLogistica(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogistica) {
  ui->setupUi(this);

  ui->splitter_6->setStretchFactor(0, 0);
  ui->splitter_6->setStretchFactor(1, 1);

  setConnections();
}

WidgetLogistica::~WidgetLogistica() { delete ui; }

void WidgetLogistica::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->tableForn, &TableView::clicked, this, &WidgetLogistica::on_tableForn_clicked, connectionType);
  connect(ui->tabWidgetLogistica, &QTabWidget::currentChanged, this, &WidgetLogistica::on_tabWidgetLogistica_currentChanged, connectionType);
}

void WidgetLogistica::resetTables() {
  ui->widgetAgendarColeta->resetTables();
  ui->widgetColeta->resetTables();
  ui->widgetRecebimento->resetTables();
  ui->widgetAgendaEntrega->resetTables();
  ui->widgetCalendarioEntrega->resetTables();
  ui->widgetCaminhao->resetTables();
  ui->widgetRepresentacao->resetTables();
  ui->widgetEntregues->resetTables();
  ui->widgetCalendario->resetTables();
}

void WidgetLogistica::updateTables() {
  ui->tableForn->setModel(&modelViewLogistica);

  const QString currentTab = ui->tabWidgetLogistica->tabText(ui->tabWidgetLogistica->currentIndex());

  ui->frameForn->hide();

  if (currentTab == "Agendar Coleta") { modelViewLogistica.setTable("view_fornecedor_logistica_agendar_coleta"); }
  if (currentTab == "Coleta") { modelViewLogistica.setTable("view_fornecedor_logistica_coleta"); }
  if (currentTab == "Recebimento") { modelViewLogistica.setTable("view_fornecedor_logistica_recebimento"); }
  if (currentTab == "Representação") { modelViewLogistica.setTable("view_fornecedor_logistica_representacao"); }

  modelViewLogistica.setFilter("");

  if (currentTab == "Agendar Coleta" or currentTab == "Coleta" or currentTab == "Recebimento" or currentTab == "Representação") {
    ui->frameForn->show();

    if (not modelViewLogistica.select()) { return; }
  }

  //--------------------------------------------------------

  if (currentTab == "Agendar Coleta") { ui->widgetAgendarColeta->updateTables(); }
  if (currentTab == "Coleta") { ui->widgetColeta->updateTables(); }
  if (currentTab == "Recebimento") { ui->widgetRecebimento->updateTables(); }
  if (currentTab == "Agendar Entrega") { ui->widgetAgendaEntrega->updateTables(); }
  if (currentTab == "Entregas") { ui->widgetCalendarioEntrega->updateTables(); }
  if (currentTab == "Caminhões") { ui->widgetCaminhao->updateTables(); }
  if (currentTab == "Representação") { ui->widgetRepresentacao->updateTables(); }
  if (currentTab == "Entregues") { ui->widgetEntregues->updateTables(); }
  if (currentTab == "Calendário") { ui->widgetCalendario->updateTables(); }
}

void WidgetLogistica::on_tableForn_clicked(const QModelIndex &index) {
  const QString fornecedor = index.isValid() ? modelViewLogistica.data(index.row(), "fornecedor").toString() : "";

  const QString currentTab = ui->tabWidgetLogistica->tabText(ui->tabWidgetLogistica->currentIndex());

  if (currentTab == "Agendar Coleta") { ui->widgetAgendarColeta->tableFornLogistica_clicked(fornecedor); }
  if (currentTab == "Coleta") { ui->widgetColeta->tableFornLogistica_clicked(fornecedor); }
  if (currentTab == "Recebimento") { ui->widgetRecebimento->tableFornLogistica_clicked(fornecedor); }
  if (currentTab == "Representação") { ui->widgetRepresentacao->tableFornLogistica_clicked(fornecedor); }
}

void WidgetLogistica::on_tabWidgetLogistica_currentChanged(const int) { updateTables(); }

// TODO: 1followup das entregas (no lugar de followup colocar campo observacao no inputDialog?)
// TODO: 5colocar aba para fazer cotacao frete, puxar os orcamentos abertos com o peso das caixas para calcular frete
// TODO: 0verificar nos cancelamentos se estou removendo as datas/previsoes corretamente
